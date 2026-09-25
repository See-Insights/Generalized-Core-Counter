#pragma once

// Host stand-in for StorageHelperRK.h, used only by
// persistence_facade_behavior_test (WO-2026-09-23-001, Step 5). It keeps the
// real src/MyPersistentData.cpp compilable and linkable on the host so the
// facade forwarders under test are the PRODUCTION ones, not a reimplementation.
//
// Values are addressed exactly as the real library does - as a byte offset
// from the SavedDataHeader pointer, i.e. straight into the caller's own
// SysData/CurrentData/SensorData member. That is what makes "two facades
// share one store" a real property here rather than an artifact of the stub:
// a facade write and a facade read of the same field touch the same bytes.
//
// The stub also records what production asked for, which is what lets the
// test assert behavior a pure getter/setter check cannot see:
//   - the dataSize argument each validate() call received (Finding 2),
//   - every flush(bool) call, so deferred (false) and forced (true)
//     persistence can be told apart (Finding 4), and
//   - WHICH STORE each flush touched. Recording only the bool left a facade
//     servicing a sibling's store (CurrentReadings::loop() flushing
//     sensor.dat) completely invisible - Stage 7 review's second finding.

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace StorageHelperRK {

struct FlushRecord {
  const char *path;
  bool forced;
};

// Test-visible recorders; defined in the test translation unit.
extern std::vector<FlushRecord> flushCalls;
extern std::vector<size_t> validateSizes;
extern int initializeCalls;

class PersistentDataBase {
public:
  class SavedDataHeader {  // 16 bytes, matching the real library exactly
  public:
    uint32_t magic;
    uint16_t version;
    uint16_t size;
    uint32_t hash;
    uint32_t reserved1;
  };
};

class PersistentDataFile : public PersistentDataBase {
public:
  PersistentDataFile(const char *path, SavedDataHeader *header, size_t size,
                     uint32_t magic, uint16_t version)
      : savedDataHeader(header), savedDataSize(size), savedDataMagic(magic),
        savedDataVersion(version), savedDataPath(path) {
    std::memset(reinterpret_cast<uint8_t *>(savedDataHeader), 0, savedDataSize);
    savedDataHeader->magic = magic;
    savedDataHeader->version = version;
    savedDataHeader->size = static_cast<uint16_t>(size);
  }
  virtual ~PersistentDataFile() {}

  PersistentDataFile &withSaveDelayMs(uint32_t ms) { saveDelayMs = ms; return *this; }
  PersistentDataFile &withLogData(bool) { return *this; }

  void load() {}
  void flush(bool forced) { flushCalls.push_back({savedDataPath, forced}); }
  void updateHash() {}

  virtual bool validate(size_t dataSize) {
    validateSizes.push_back(dataSize);
    return true;
  }

  virtual void initialize() {
    initializeCalls++;
    uint8_t *p = reinterpret_cast<uint8_t *>(savedDataHeader);
    std::memset(p + sizeof(SavedDataHeader), 0, savedDataSize - sizeof(SavedDataHeader));
  }

  template <typename T>
  T getValue(size_t offset) const {
    T result{};
    if (offset <= (savedDataSize - sizeof(T))) {
      const uint8_t *p = reinterpret_cast<const uint8_t *>(savedDataHeader) + offset;
      std::memcpy(&result, p, sizeof(T));
    }
    return result;
  }

  template <typename T>
  void setValue(size_t offset, T value) {
    if (offset <= (savedDataSize - sizeof(T))) {
      uint8_t *p = reinterpret_cast<uint8_t *>(savedDataHeader) + offset;
      std::memcpy(p, &value, sizeof(T));
    }
  }

  void getValueString(size_t offset, size_t size, String &result) const {
    const char *p = reinterpret_cast<const char *>(savedDataHeader) + offset;
    char buf[256] = {0};
    std::strncpy(buf, p, size < sizeof(buf) ? size : sizeof(buf) - 1);
    result = String(buf);
  }

  bool setValueString(size_t offset, size_t size, const char *str) {
    if (!str || std::strlen(str) >= size) return false;
    char *p = reinterpret_cast<char *>(savedDataHeader) + offset;
    std::memset(p, 0, size);
    std::memcpy(p, str, std::strlen(str));
    return true;
  }

  uint32_t saveDelayMs = 0;

protected:
  SavedDataHeader *savedDataHeader;
  size_t savedDataSize;
  uint32_t savedDataMagic;
  uint16_t savedDataVersion;
  const char *savedDataPath;
};

}  // namespace StorageHelperRK
