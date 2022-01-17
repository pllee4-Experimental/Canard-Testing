/*
 * canard_rx.cpp
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

void CanardDecode(CanardTransfer *transfer) {
  uint8_t buffer[4096] = {};
  size_t buffer_size = 4096;
  switch (transfer->port_id) {
    case 0x121:
      std::cout << "Received data" << std::endl;
      uint8_t *data =
          static_cast<uint8_t *>(const_cast<void *>(transfer->payload));
      for (uint8_t i = 0; i < 8; ++i) {
        printf("Payload[%d]: %d\n", i, data[i]);
      }
      printf("\n");
      break;
  }
}

void Receive(CanardInstance *inst) {
  const char *const iface_name = "vcan0";
  const auto sb = socketcanOpen(iface_name, false);
  if (sb < 0) {
    std::cout << "failed to open can: " << iface_name << std::endl;
  }
  // setup receive
  CanardRxSubscription test_subscription;
  (void)canardRxSubscribe(inst,  // Subscribe to messages uavcan.node.Heartbeat.
                          CanardTransferKindMessage,
                          0x121,  // The fixed Subject-ID of the Heartbeat
                                  // message type (see DSDL definition).
                          1024,   // The extent (the maximum possible payload
                                  // size); pick a huge value if not sure.
                          CANARD_DEFAULT_TRANSFER_ID_TIMEOUT_USEC,
                          &test_subscription);

  // setup transfer
  std::cout << "rx thread started" << std::endl;

  char buf[4096]{};
  CanardFrame frame{};

  while (true) {
    if (socketcanPop(sb, &frame, sizeof(buf), buf, 0, nullptr) == 1) {
      static CanardTransfer transfer;
      const int8_t result = canardRxAccept(inst, &frame, 0, &transfer);
      if (result == 1) {
        CanardDecode(&transfer);
        inst->memory_free(inst, (void *)transfer.payload);
      }
    }
    // usleep(1000);
  }
}

int main(int argc, char *argv[]) {
  CanardInstance inst = canardInit(MemoryAllocate, MemoryFree);
  inst.mtu_bytes = CANARD_MTU_CAN_CLASSIC;
  inst.node_id = 42;
  std::thread receive_thread(Receive, &inst);

  while (true) {
  }

  return 0;
}