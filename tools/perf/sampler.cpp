// Sampling profiler for a running process (Windows, no admin rights needed).
//
//   sampler <pid> <seconds> <interval_ms> <out.txt> [max_threads] [focus,focus,...]
//
// A first second measures the CPU time per thread; then only the busiest threads are
// sampled: suspend, read the context, walk the stack with dbghelp, resume. The report has,
// per thread, the functions by self time (the innermost frame) and by inclusive time
// (anywhere on the stack), and the most common stacks. For each focus (part of a function
// name) it also lists what that function calls and the innermost frames under it.
//
// Function names need PDBs: link the SDK and the game with -Wl,/DEBUG,/OPT:REF,/OPT:ICF
// (see docs/DEVELOPMENT.md). Names of system DLLs without symbols are the nearest
// export, so memcpy in vcruntime140 shows up as _NLG_Return2.
//
// Build: clang++ -O2 -std=c++20 tools/perf/sampler.cpp -o sampler.exe -ldbghelp

#include <windows.h>
#include <dbghelp.h>
#include <tlhelp32.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

constexpr int kMaxFrames = 48;

struct Sample {
  int count = 0;
  DWORD64 pcs[kMaxFrames];
};

struct ThreadInfo {
  DWORD id = 0;
  HANDLE handle = nullptr;
  ULONGLONG cpu_before = 0;
  ULONGLONG cpu_start = 0;
  ULONGLONG cpu_end = 0;
  double share = 0;
  std::wstring name;
  std::vector<Sample> samples;
};

ULONGLONG CpuTime(HANDLE thread) {
  FILETIME creation, exit, kernel, user;
  if (!GetThreadTimes(thread, &creation, &exit, &kernel, &user)) {
    return 0;
  }
  return (ULONGLONG(kernel.dwHighDateTime) << 32 | kernel.dwLowDateTime) +
         (ULONGLONG(user.dwHighDateTime) << 32 | user.dwLowDateTime);
}

HANDLE g_process = nullptr;
std::unordered_map<DWORD64, std::string> g_symbols;

const std::string& Symbol(DWORD64 pc) {
  auto it = g_symbols.find(pc);
  if (it != g_symbols.end()) {
    return it->second;
  }
  char buffer[sizeof(SYMBOL_INFO) + 1024];
  auto* info = reinterpret_cast<SYMBOL_INFO*>(buffer);
  info->SizeOfStruct = sizeof(SYMBOL_INFO);
  info->MaxNameLen = 1000;
  DWORD64 displacement = 0;
  std::string text;
  IMAGEHLP_MODULE64 module = {};
  module.SizeOfStruct = sizeof(module);
  const bool has_module = SymGetModuleInfo64(g_process, pc, &module);
  if (has_module) {
    text = module.ModuleName;
    text += "!";
  }
  if (SymFromAddr(g_process, pc, &displacement, info)) {
    text += info->Name;
  } else {
    char raw[64];
    snprintf(raw, sizeof(raw), "0x%llX", has_module ? pc - module.BaseOfImage : pc);
    text += raw;
  }
  return g_symbols.emplace(pc, std::move(text)).first->second;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 5) {
    fprintf(stderr, "usage: sampler <pid> <seconds> <interval_ms> <out.txt> [max_threads]\n");
    return 2;
  }
  const DWORD pid = DWORD(atoi(argv[1]));
  const double seconds = atof(argv[2]);
  const int interval_ms = atoi(argv[3]);
  const char* out_path = argv[4];
  const size_t max_threads = argc > 5 ? size_t(atoi(argv[5])) : 8;
  std::vector<std::string> focuses;
  if (argc > 6) {
    std::string list = argv[6];
    size_t begin = 0;
    while (begin <= list.size()) {
      const size_t found = list.find(',', begin);
      const size_t end = found == std::string::npos ? list.size() : found;
      if (end > begin) {
        focuses.push_back(list.substr(begin, end - begin));
      }
      begin = end + 1;
    }
  }

  g_process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
  if (!g_process) {
    fprintf(stderr, "OpenProcess failed: %lu\n", GetLastError());
    return 1;
  }

  std::vector<ThreadInfo> threads;
  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
  THREADENTRY32 entry = {};
  entry.dwSize = sizeof(entry);
  for (BOOL ok = Thread32First(snapshot, &entry); ok; ok = Thread32Next(snapshot, &entry)) {
    if (entry.th32OwnerProcessID != pid) {
      continue;
    }
    ThreadInfo info;
    info.id = entry.th32ThreadID;
    info.handle = OpenThread(
        THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE, info.id);
    if (!info.handle) {
      continue;
    }
    PWSTR description = nullptr;
    if (SUCCEEDED(GetThreadDescription(info.handle, &description)) && description) {
      info.name = description;
      LocalFree(description);
    }
    info.cpu_before = CpuTime(info.handle);
    threads.push_back(std::move(info));
  }
  CloseHandle(snapshot);

  Sleep(1000);
  for (auto& thread : threads) {
    thread.share = double(CpuTime(thread.handle) - thread.cpu_before) / 1e7;
  }
  std::sort(threads.begin(), threads.end(),
            [](const ThreadInfo& a, const ThreadInfo& b) { return a.share > b.share; });
  size_t sampled = 0;
  while (sampled < threads.size() && sampled < max_threads && threads[sampled].share > 0.01) {
    ++sampled;
  }

  SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_FAIL_CRITICAL_ERRORS);
  if (!SymInitialize(g_process, nullptr, TRUE)) {
    fprintf(stderr, "SymInitialize failed: %lu\n", GetLastError());
    return 1;
  }

  LARGE_INTEGER frequency, start, now;
  QueryPerformanceFrequency(&frequency);
  QueryPerformanceCounter(&start);
  for (size_t i = 0; i < sampled; ++i) {
    threads[i].cpu_start = CpuTime(threads[i].handle);
  }
  ULONGLONG in_suspend = 0;
  int rounds = 0;
  for (;;) {
    QueryPerformanceCounter(&now);
    if (double(now.QuadPart - start.QuadPart) / double(frequency.QuadPart) >= seconds) {
      break;
    }
    for (size_t i = 0; i < sampled; ++i) {
      ThreadInfo& thread = threads[i];
      LARGE_INTEGER t0, t1;
      QueryPerformanceCounter(&t0);
      if (SuspendThread(thread.handle) == DWORD(-1)) {
        continue;
      }
      CONTEXT context = {};
      context.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
      Sample sample;
      if (GetThreadContext(thread.handle, &context)) {
        STACKFRAME64 frame = {};
        frame.AddrPC.Offset = context.Rip;
        frame.AddrPC.Mode = AddrModeFlat;
        frame.AddrFrame.Offset = context.Rbp;
        frame.AddrFrame.Mode = AddrModeFlat;
        frame.AddrStack.Offset = context.Rsp;
        frame.AddrStack.Mode = AddrModeFlat;
        while (sample.count < kMaxFrames &&
               StackWalk64(IMAGE_FILE_MACHINE_AMD64, g_process, thread.handle, &frame, &context,
                           nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr)) {
          if (frame.AddrPC.Offset == 0) {
            break;
          }
          sample.pcs[sample.count++] = frame.AddrPC.Offset;
        }
      }
      ResumeThread(thread.handle);
      QueryPerformanceCounter(&t1);
      in_suspend += ULONGLONG(t1.QuadPart - t0.QuadPart);
      if (sample.count) {
        thread.samples.push_back(sample);
      }
    }
    ++rounds;
    Sleep(DWORD(interval_ms));
  }
  QueryPerformanceCounter(&now);
  const double elapsed = double(now.QuadPart - start.QuadPart) / double(frequency.QuadPart);
  for (size_t i = 0; i < sampled; ++i) {
    threads[i].cpu_end = CpuTime(threads[i].handle);
  }

  FILE* out = nullptr;
  if (fopen_s(&out, out_path, "w") != 0 || !out) {
    return 1;
  }
  fprintf(out, "pid %lu, %.1f s, %d rounds, %.1f%% of the time spent holding a thread\n", pid,
          elapsed, rounds, 100.0 * double(in_suspend) / double(frequency.QuadPart) / elapsed);
  for (size_t i = 0; i < sampled; ++i) {
    ThreadInfo& thread = threads[i];
    const double cpu = double(thread.cpu_end - thread.cpu_start) / 1e7 / elapsed;
    fprintf(out, "\n==== thread %lu '%ls': %.0f%% CPU, %zu samples ====\n", thread.id,
            thread.name.c_str(), cpu * 100.0, thread.samples.size());
    if (thread.samples.empty()) {
      continue;
    }
    std::map<std::string, int> self, inclusive, stacks;
    for (const Sample& sample : thread.samples) {
      self[Symbol(sample.pcs[0])]++;
      std::vector<std::string> seen;
      std::string stack;
      for (int f = 0; f < sample.count; ++f) {
        const std::string& name = Symbol(sample.pcs[f]);
        if (std::find(seen.begin(), seen.end(), name) == seen.end()) {
          seen.push_back(name);
          inclusive[name]++;
        }
        if (f < 14) {
          stack += name;
          stack += " < ";
        }
      }
      stacks[stack]++;
    }
    const auto print = [&](const char* title, const std::map<std::string, int>& table,
                           size_t limit) {
      std::vector<std::pair<int, const std::string*>> order;
      for (const auto& [name, count] : table) {
        order.emplace_back(count, &name);
      }
      std::sort(order.begin(), order.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
      fprintf(out, "-- %s --\n", title);
      for (size_t n = 0; n < order.size() && n < limit; ++n) {
        fprintf(out, "%6.1f%%  %s\n", 100.0 * order[n].first / double(thread.samples.size()),
                order[n].second->c_str());
      }
    };
    print("self", self, 45);
    print("inclusive", inclusive, 70);
    print("stacks", stacks, 14);
    // For each function named in argv[6] (comma separated substrings): what it calls
    // directly, and the innermost frames under it.
    for (const std::string& focus : focuses) {
      std::map<std::string, int> children, leaves;
      int total = 0;
      for (const Sample& sample : thread.samples) {
        for (int f = 0; f < sample.count; ++f) {
          if (Symbol(sample.pcs[f]).find(focus) == std::string::npos) {
            continue;
          }
          ++total;
          children[f > 0 ? Symbol(sample.pcs[f - 1]) : std::string("(self)")]++;
          leaves[Symbol(sample.pcs[0])]++;
          break;
        }
      }
      if (!total) {
        continue;
      }
      fprintf(out, "-- focus %s: %.1f%% --\n", focus.c_str(),
              100.0 * total / double(thread.samples.size()));
      print("  calls", children, 25);
      print("  leaves", leaves, 15);
    }
  }
  fclose(out);
  SymCleanup(g_process);
  return 0;
}
