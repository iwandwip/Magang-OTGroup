#ifndef COMMAND_QUEUE_H
#define COMMAND_QUEUE_H

#include <Arduino.h>

class CommandQueue {
public:
  CommandQueue(int maxSize = 5);
  
  void clear();
  bool isEmpty() const;
  bool isFull() const;
  int size() const;
  
  bool add(const String& command);
  String get();
  
private:
  static const int DEFAULT_MAX_SIZE = 5;
  
  String* commandQueue;
  int maxQueueSize;
  int queueHead;
  int queueTail;
  int queueSize;
};

#endif