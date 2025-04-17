#include "MotionQueue.h"

MotionQueue::MotionQueue(int maxSize)
  : maxMotions(maxSize) {
  motionQueue = new MotionStep[maxMotions];
  clear();
}

void MotionQueue::clear() {
  queuedMotionsCount = 0;
  currentMotionIndex = 0;

  for (int i = 0; i < maxMotions; i++) {
    motionQueue[i].position = 0;
    motionQueue[i].speed = 0;
    motionQueue[i].delayMs = 0;
    motionQueue[i].isDelayOnly = false;
    motionQueue[i].completed = false;
  }
}

bool MotionQueue::isEmpty() const {
  return queuedMotionsCount == 0;
}

bool MotionQueue::isFull() const {
  return queuedMotionsCount >= maxMotions;
}

int MotionQueue::size() const {
  return queuedMotionsCount;
}

int MotionQueue::capacity() const {
  return maxMotions;
}

bool MotionQueue::add(long position, float speed, unsigned long delayMs, bool isDelayOnly) {
  if (isFull()) {
    return false;
  }

  motionQueue[queuedMotionsCount].position = position;
  motionQueue[queuedMotionsCount].speed = speed;
  motionQueue[queuedMotionsCount].delayMs = delayMs;
  motionQueue[queuedMotionsCount].isDelayOnly = isDelayOnly;
  motionQueue[queuedMotionsCount].completed = false;

  queuedMotionsCount++;
  return true;
}

bool MotionQueue::add(const MotionStep& step) {
  return add(step.position, step.speed, step.delayMs, step.isDelayOnly);
}

MotionQueue::MotionStep* MotionQueue::current() {
  if (isEmpty() || currentMotionIndex >= queuedMotionsCount) {
    return nullptr;
  }
  return &motionQueue[currentMotionIndex];
}

MotionQueue::MotionStep* MotionQueue::get(int index) {
  if (index < 0 || index >= queuedMotionsCount) {
    return nullptr;
  }
  return &motionQueue[index];
}

bool MotionQueue::moveToNext() {
  if (currentMotionIndex < queuedMotionsCount - 1) {
    currentMotionIndex++;
    return true;
  }
  return false;
}

int MotionQueue::currentIndex() const {
  return currentMotionIndex;
}