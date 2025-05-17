#include <vector>
#include <string>
#include <stdexcept>
#include <cstdint>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <wasm_export.h>


// read WASM bytecode at runtime (into byte array)
std::vector<uint8_t> readFileToBytes(const std::string& path)
{
  int fd = open(path.c_str(), O_RDONLY);
  if (fd < 0) {
    throw std::runtime_error("Couldn't open file " + path);
  }
  struct stat statbuf;
  int staterr = fstat(fd, &statbuf);
  if (staterr < 0) {
    throw std::runtime_error("Couldn't stat file " + path);
  }
  ssize_t fsize = statbuf.st_size;
  posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);
  std::vector<uint8_t> result;
  result.resize(fsize);
  int cpos = 0;
  while (cpos < fsize) {
    int rc = read(fd, result.data(), fsize - cpos);
    if (rc < 0) {
      perror("Couldn't read file");
      throw std::runtime_error("Couldn't read file " + path);
    } else {
      cpos += rc;
    }
  }
  close(fd);
  return result;
}

// read wasm file, load module and create execution environment
wasm_module_t load_module_minimal(
  std::vector<uint8_t>& buffer,
  wasm_module_inst_t& out_inst,
  wasm_exec_env_t& out_env,
  uint32_t stack_size,
  uint32_t heap_size,
  char* error_buf,
  size_t error_buf_size) {

  wasm_module_t module = wasm_runtime_load(buffer.data(), buffer.size(), error_buf, error_buf_size);
  if (!module) {
    printf("Load wasm module failed. error: %s\n", error_buf);
    return nullptr;
  }

  out_inst = wasm_runtime_instantiate(module, stack_size, heap_size, error_buf, error_buf_size);
  if (!out_inst) {
    printf("Instantiate wasm module failed. error: %s\n", error_buf);
    wasm_runtime_unload(module);
    return nullptr;
  }

  out_env = wasm_runtime_create_exec_env(out_inst, stack_size);
  if (!out_env) {
    printf("Create wasm execution environment failed.\n");
    wasm_runtime_deinstantiate(out_inst);
    wasm_runtime_unload(module);
    return nullptr;
  }

  return module;
}