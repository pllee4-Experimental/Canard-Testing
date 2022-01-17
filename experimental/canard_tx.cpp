/*
 * canard_tx.cpp
 * Created on: Jan 17, 2022 22:09
 * Description:
 *
 * Copyright (c) 2022 Pin Loon Lee (pllee4)
 */

#include "assert.h"
#define NUNAVUT_ASSERT(x) assert(x)

#include <unistd.h>

#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>

#include "canard.h"
#include "socketcan.h"

#define RESERVED_MEMORY_SIZE (1024 * 256 * (sizeof(void *)))

std::mutex heap_mutex;

void *MemoryAllocate(CanardInstance *ins, size_t amount) {
  return malloc(amount * (sizeof(uint8_t)));
}
void MemoryFree(CanardInstance *ins, void *pointer) { free(pointer); }

void Transmit(CanardInstance *inst) {
  const char *const iface_name = "vcan0";
  const auto sa = socketcanOpen(iface_name, false);
  if (sa < 0) {
    std::cout << "failed to open can: " << iface_name << std::endl;
  }
  std::cout << "tx thread started" << std::endl;

  while (true) {
    heap_mutex.lock();
    const CanardFrame *frame = canardTxPeek(inst);
    heap_mutex.unlock();
    if (frame != NULL) {
      socketcanPush(sa, frame, 1'000'000);
      std::cout << "sending: " << frame->extended_can_id
                << " , size: " << frame->payload_size << std::endl;
      for (int i = 0; i < frame->payload_size; ++i) {
        std::cout << std::hex
                  << static_cast<uint16_t>(
                         static_cast<const uint8_t *>(frame->payload)[i])
                  << " " << std::endl;
      }
      std::cout << std::endl;
      heap_mutex.lock();
      canardTxPop(inst);
      heap_mutex.unlock();
      inst->memory_free(inst, (CanardFrame *)frame);
    }
    // usleep(1000);
  }
}

int main(int argc, char *argv[]) {
  uint8_t mem[RESERVED_MEMORY_SIZE];

  uint8_t buffer[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
  size_t buffer_size = 1024;

  CanardInstance inst = canardInit(MemoryAllocate, MemoryFree);
  inst.mtu_bytes = CANARD_MTU_CAN_CLASSIC;
  inst.node_id = 42;

  std::thread transmit_thread(Transmit, &inst);

  // setup transfer
  CanardTransfer transfer;
  transfer.priority = CanardPriorityNominal;
  transfer.transfer_kind = CanardTransferKindMessage;
  transfer.port_id = 0x121;
  transfer.remote_node_id = CANARD_NODE_ID_UNSET;
  transfer.transfer_id = 0;
  transfer.payload_size = buffer_size;
  transfer.payload = buffer;

  std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();
  while (true) {
    transfer.transfer_id++;

    heap_mutex.lock();
    canardTxPush(&inst, &transfer);
    heap_mutex.unlock();
    usleep(5000);
  }

  return 0;
}