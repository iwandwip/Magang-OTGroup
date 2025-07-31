#ifndef MOTION_QUEUE_H
#define MOTION_QUEUE_H

class MotionQueue {
public:
  struct MotionStep {
    long position;
    float speed;
    unsigned long delayMs;
    bool isDelayOnly;
    bool completed;
  };

  MotionQueue(int maxSize = 5);
  
  void clear();
  bool isEmpty() const;
  bool isFull() const;
  int size() const;
  int capacity() const;
  
  bool add(long position, float speed, unsigned long delayMs = 0, bool isDelayOnly = false);
  bool add(const MotionStep& step);
  
  MotionStep* current();
  MotionStep* get(int index);
  bool moveToNext();
  int currentIndex() const;

private:
  MotionStep* motionQueue;
  int maxMotions;
  int queuedMotionsCount;
  int currentMotionIndex;
};

#endif