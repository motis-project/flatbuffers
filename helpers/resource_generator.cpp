#include <cstring>
#include <cstdio>
#include <istream>
#include <ostream>
#include <fstream>
#include <vector>
#include <map>
#include <algorithm>

#include "flatbuffers/util.h"
#include "flatbuffers/namespace.h"

std::vector<std::string> dependencies(std::vector<std::string> const& lines) {
  std::vector<std::string> dep;

  for (auto const& line : lines) {
    if (strncmp(line.c_str(), "include", 7) == 0) {
      auto filename_start = line.find('"');
      if (filename_start == std::string::npos) {
        continue;
      }

      auto filename_end = line.find_last_of('"');
      if (filename_end == std::string::npos || filename_start == filename_end) {
        continue;
      }

      auto filename_length = filename_end - filename_start - 1;
      auto filename = line.substr(filename_start + 1, filename_length);

      dep.emplace_back(std::move(filename));
    }
  }

  return dep;
}

std::vector<std::string> read_file(std::string const& path) {
  std::vector<std::string> lines;

  std::ifstream in;
  in.exceptions(std::ifstream::failbit | std::ifstream::badbit);

  try {
    in.open(path.c_str());

    std::string line;
    while (!in.eof() && in.peek() != EOF) {
      std::getline(in, line);
      lines.emplace_back(std::move(line));
    }
  } catch (std::ios_base::failure const&) {
    fprintf(stderr, "unable to read file \"%s\"\n", path.c_str());
    throw;
  }

  return lines;
}

std::string root_path(std::string const& file_path) {
  std::string path = ".";
  std::string root_file = file_path;
  auto filename_begin = root_file.find_last_of('/');
  if (filename_begin != std::string::npos) {
    path = root_file.substr(0, filename_begin + 1);
  }
  return path;
}

std::string join(std::vector<std::string> const& lines) {
  std::string merged;
  for (auto const& line : lines) {
    merged.append(line);
    merged.append("\n");
  }
  return merged;
}

void build_order(std::string const& root_path, std::string const& path,
                 std::string const& filename, std::vector<std::string>& order,
                 std::map<std::string, std::string>& file_contents) {
  auto lines = read_file(path);
  file_contents[filename] = join(lines);
  for (auto const& dep : dependencies(lines)) {
    build_order(root_path, root_path + dep, dep, order, file_contents);
  }
  if (std::find(begin(order), end(order), filename) == end(order)) {
    order.push_back(filename);
  }
}

void replace_all(std::string& s, std::string const& from,
                 std::string const& to) {
  std::string::size_type pos;
  while ((pos = s.find(from)) != std::string::npos) {
    s.replace(pos, from.size(), to);
  }
}

std::string name_to_identifier(std::string const& name) {
  std::string identifier = name;
  replace_all(identifier, "/", "_");
  replace_all(identifier, ".", "_");
  return identifier;
}

std::string data_to_binary_array(std::string const& data) {
  std::string s;
  s.append("{");
  for (auto const& d : data) {
    s.append("0x");
    s.append(flatbuffers::IntToStringHex(d, 2));
    s.append(", ");
  }
  s.append("0x00");
  s.append("}");
  return s;
}

std::string resource_string(std::string const& name, std::string const& data) {
  std::string s;
  s.append("static const char ");
  s.append(name_to_identifier(name));
  s.append("[] = ");
  s.append(data_to_binary_array(data));
  s.append(";\n");
  return s;
}

std::string resource_file(
    std::vector<std::string> const& order,
    std::map<std::string, std::string> const& file_contents) {
  std::string f;
  for (auto const& path : order) {
    f.append(resource_string(path, file_contents.at(path)));

    f.append("static const char ");
    f.append(name_to_identifier(path));
    f.append("_filename[] = \"");
    f.append(path);
    f.append("\";\n");
  }

  f.append("static const char* filenames[] = {");
  for (auto const& path : order) {
    f.append(name_to_identifier(path));
    f.append("_filename");
    f.append(", ");
  }
  f.append("};\n");

  f.append("const char* symbols[] = {");
  for (auto const& symbol : order) {
    f.append(name_to_identifier(symbol));
    f.append(",");
  }
  f.append("};\n");

  f.append("static const unsigned int number_of_symbols = ");
  f.append(flatbuffers::NumToString(order.size()));
  f.append(";\n");

  return f;
}

std::vector<std::string> parse_namespaces(std::string const& input) {
  std::string delim = "::";
  std::vector<std::string> namespaces;

  auto start = 0u;
  auto end = input.find(delim);
  while (end != std::string::npos) {
    if (end - start > 0) {
      namespaces.emplace_back(input.substr(start, end - start));
    }
    start = end + delim.length();
    end = input.find(delim, start);
  }

  if (end - start > 0) {
    namespaces.emplace_back(input.substr(start, end - start));
  }

  return namespaces;
}

int main(int argc, char** argv) {
  if (argc != 3 && argc != 4) {
    printf("usage: %s <path to fbs root type> <target> [namespace]\n", argv[0]);
    return 0;
  }

  std::vector<std::string> namespaces;
  if (argc >= 4) {
    namespaces = parse_namespaces(argv[3]);
  }

  std::string root_file = argv[1];
  auto path = root_path(root_file);
  auto filename = root_file.substr(path.length());

  std::map<std::string, std::string> file_contents;
  std::vector<std::string> order;
  build_order(path, root_file, filename, order, file_contents);

  try {
    std::ofstream out;
    out.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    out.open(argv[2], std::ofstream::out);

    for (auto const& ns : namespaces) {
      out << "namespace " << ns << " {\n";
    }

    out << resource_file(order, file_contents);

    for (auto i = namespaces.size(); i > 0; --i) {
      out << "}  // namespace " << namespaces[i - 1] << "\n";
    }
  } catch (std::ios_base::failure const&) {
    printf("could not open %s for writing\n", argv[2]);
    return 1;
  }
}