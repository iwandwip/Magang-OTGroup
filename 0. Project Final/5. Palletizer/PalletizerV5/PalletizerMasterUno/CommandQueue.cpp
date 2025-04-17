#include "CommandQueue.h"

CommandQueue::CommandQueue(int maxSize)
  : maxQueueSize(maxSize > 0 ? maxSize : DEFAULT_MAX_SIZE),
    queueHead(0),
    queueTail(0),
    queueSize(0) {
  commandQueue = new String[maxQueueSize];
}

void CommandQueue::clear() {
  queueHead = 0;
  queueTail = 0;
  queueSize = 0;
}

bool CommandQueue::isEmpty() const {
  return queueSize == 0;
}

bool CommandQueue::isFull() const {
  return queueSize >= maxQueueSize;
}

int CommandQueue::size() const {
  return queueSize;
}

bool CommandQueue::add(const String& command) {
  if (isFull()) {
    return false;
  }

  commandQueue[queueTail] = command;
  queueTail = (queueTail + 1) % maxQueueSize;
  queueSize++;

  return true;
}

String CommandQueue::get() {
  if (isEmpty()) {
    return "";
  }

  String command = commandQueue[queueHead];
  queueHead = (queueHead + 1) % maxQueueSize;
  queueSize--;

  return command;
}