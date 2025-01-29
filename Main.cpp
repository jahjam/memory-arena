#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <new>
#include <vector>

#define ArenaPush(arena, type) static_cast<type *>(arena._push(sizeof(type)))
#define ArenaPushStringWithSize(arena, type, size)                             \
  static_cast<type *>(arena._push(size))

struct Chunk {
  char *data;
};

class Arena {
private:
  size_t _chunkCapacity;
  size_t _totalSize;
  size_t _memoryChunkRemaining;
  size_t _top;
  size_t _alignment;
  std::vector<Chunk> _memoryChain;
  size_t _curChunk;

public:
  Arena(size_t cap = 1024)
      : _chunkCapacity(cap), _top(0), _memoryChunkRemaining(cap),
        _alignment(alignof(max_align_t)), _totalSize(_chunkCapacity) {
    char *mem = static_cast<char *>(std::malloc(cap));

    if (!mem) {
      throw std::bad_alloc();
    }

    _memoryChain.push_back({.data = mem});
    _curChunk = 0;
  };

  ~Arena() {
    for (auto chunk : _memoryChain) {
      std::free(chunk.data);
    }
    _memoryChain.clear();
    _chunkCapacity = 0;
    _top = 0;
  };

  void _setMaxAlign() { _alignment = alignof(max_align_t); }

  void _setAutoAlign(size_t align) {
    // align needs to be a power of 2 to
    // to keep things efficient and
    // hardware friendly
    //
    // Binary of 8: 1000
    // Binary of 8−1: 0111
    // 8 & 7 = 1000 & 0111 = 0000 (Result is 0 → Valid power of two)
    //
    // Case 2: alignment = 5 (not a power of two)
    //
    // Binary of 5: 0101
    // Binary of 5−1: 0100
    // 5 & 4 = 0101 & 0100 = 0100 (Result is not 0 → Not a power of two)
    assert((_alignment & (_alignment - 1)) == 0 &&
           "Alignment must be a power of 2");

    _alignment = align;
  };

  size_t _pos(Arena *arena) { return _top; };

  size_t _getMemoryRemaining() { return _memoryChunkRemaining; }

  // function to allocate but not zero the memory
  void *_pushNoZero(size_t size) {
    assert(size < _chunkCapacity &&
           "Size of type larged that size of arena chunk");

    // padding moves unaligned top pointer
    // to the next aligned place in relation
    // to the _alignment value
    size_t padding;
    // we don't need padding if _top
    // is already multiple of the _alignment
    if (_top % _alignment == 0) {
      padding = 0;
    } else {
      // _alignment = 8
      // _top = 13
      // 13 % 8 = 5
      // 8 - 5 = 3
      padding = _alignment - (_top % _alignment);
    }

    // check out of bounds
    // and add a chunk if gone beyond bounds
    if (_top + padding + size > _chunkCapacity) {
      _memoryChain.push_back(
          {.data = static_cast<char *>(std::malloc(_chunkCapacity))});
      _curChunk++;
      _top = 0;
      _memoryChunkRemaining = _chunkCapacity;
      _totalSize += _chunkCapacity;
      padding = 0;
    }

    // from the _memory pointer, we
    // move it the desired allocSize
    // _top = 6;
    // padding = 2
    // 0x000 + 8 = 0x008
    // ptr = the next block of memory
    // and the first 8 bytes are used
    size_t topAndPadding = _top + padding;
    void *nextPosition = _memoryChain[_curChunk].data + topAndPadding;
    // move _top to represent the size of
    // the alloc that has occured
    // _top = 6
    // size = 12
    // padding = 2
    // 6 + 12 + 2
    // _top = 20
    // next time we call to push
    // padding will have to be applied
    // we could correct padding here
    // but it's no different either
    // do the padding before or after
    _top += padding + size;
    _memoryChunkRemaining = _chunkCapacity - _top;
    return nextPosition;
  };

  void *_pushAligner(size_t alignment) {
    assert((alignment & (alignment - 1)) == 0 &&
           "Alignment must be a power of 2");
    size_t padding;

    if (_top % alignment == 0) {
      padding = 0;
    } else {
      padding = alignment - (_top % alignment);
    }

    size_t topAndPadding = _top + padding;

    if (topAndPadding > _chunkCapacity) {
      _memoryChain.push_back(
          {.data = static_cast<char *>(std::malloc(_chunkCapacity))});
      _curChunk++;
      _top = 0;
      _memoryChunkRemaining = _chunkCapacity;
      _totalSize += _chunkCapacity;
      padding = 0;
    }

    _top = topAndPadding;
    void *nextPosition = _memoryChain[_curChunk].data + _top;
    return nextPosition;
  };

  void *_push(size_t size) {
    void *ptr = _pushNoZero(size);
    // zero the memory
    void *alloc = std::memset(ptr, 0, size);
    return alloc;
  };

  void _pop(size_t size) {
    if (size >= _top && _curChunk != 0) {
      _memoryChain.erase(_memoryChain.begin() + _curChunk);
      _curChunk--;
      _top = _chunkCapacity;
      _memoryChunkRemaining = 0;
      _totalSize -= _chunkCapacity;
      return;
    };

    assert(size <= _top && "Can't pop off more size than is used");

    _top -= size;
    _memoryChunkRemaining += size;
  };

  void _clear() {
    if (_curChunk > 0) {
      for (auto it = _memoryChain.begin() + 1; it != _memoryChain.end(); ++it) {
        std::free(it->data);
        it = _memoryChain.erase(it);
      }
      _curChunk = 0;
    }

    _top = 0;
    _memoryChunkRemaining = _chunkCapacity;
  };
};

int main(int argc, char *argv[]) {
  Arena arena(16);

  arena._setAutoAlign(sizeof(int));
  int *intP1 = ArenaPush(arena, int);
  int *intP2 = ArenaPush(arena, int);
  int *intP3 = ArenaPush(arena, int);
  *intP1 = 5;
  *intP2 = 10;
  *intP3 = 20;
  //
  //  std::string myStr = "Hello world! Welcome to the memory arena!";
  //  size_t strLength = myStr.size();
  //  arena._setMaxAlign();
  //  char *strP = ArenaPushStringWithSize(arena, char, strLength);
  //  std::strncpy(strP, myStr.c_str(), strLength);

  arena._setAutoAlign(sizeof(double));
  double *doubleP = ArenaPush(arena, double);
  *doubleP = 1.5;

  arena._pop(sizeof(double));
  arena._pop(sizeof(int));
  arena._pop(sizeof(int));
  arena._pop(sizeof(int));

  return 0;
}
