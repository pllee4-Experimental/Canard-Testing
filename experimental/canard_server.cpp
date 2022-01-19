/*
 * canard_server.cpp
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

std::mutex heap_mutex;

void *MemoryAllocate(CanardInstance *ins, size_t amount) {
  return malloc(amount * (sizeof(uint8_t)));
}
void MemoryFree(CanardInstance *ins, void *pointer) { free(pointer); }

void HandleRequestTransfer(CanardInstance *inst,
                           CanardTransfer *request_transfer) {
  uint8_t payload[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  CanardTransfer response_transfer = *request_transfer;
  response_transfer.transfer_kind = CanardTransferKindResponse;
  response_transfer.payload_size = 10;  // sizeof(payload)
  response_transfer.payload = payload;
  canardTxPush(inst, &response_transfer);
}

void Server(CanardInstance *inst) {
  const char *const iface_name = "vcan0";
  const auto sa = socketcanOpen(iface_name, false);
  if (sa < 0) {
    std::cout << "failed to open can: " << iface_name << std::endl;
  }
  // setup receive
  CanardRxSubscription test_subscription;
  (void)canardRxSubscribe(inst,  // Subscribe to messages uavcan.node.Heartbeat.
                          CanardTransferKindRequest,
                          100,   // Service ID
                          1024,  // The extent (the maximum possible payload
                                 // size); pick a huge value if not sure.
                          CANARD_DEFAULT_TRANSFER_ID_TIMEOUT_USEC,
                          &test_subscription);

  std::cout << "server thread started" << std::endl;

  char buf[4096]{};
  CanardFrame rcv_frame{};

  while (true) {
    const CanardFrame *send_frame = canardTxPeek(inst);
    if (send_frame != NULL) {
      socketcanPush(sa, send_frame, 1'000'000);
      canardTxPop(inst);
      inst->memory_free(inst, (CanardFrame *)send_frame);
    }

    if (socketcanPop(sa, &rcv_frame, sizeof(buf), buf, 0, nullptr) == 1) {
      static CanardTransfer transfer;
      const int8_t result = canardRxAccept(inst, &rcv_frame, 0, &transfer);
      if (result == 1) {
        printf("transfer_kind %d port_id %d\n", transfer.transfer_kind,
               transfer.port_id);
        if ((transfer.transfer_kind == CanardTransferKindRequest) &&
            (transfer.port_id == 100)) {
          std::cout << "Received request" << std::endl;
          HandleRequestTransfer(inst, &transfer);
        }
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

  std::thread server_thread(Server, &inst);

  std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();
  while (true) {
  }

  return 0;
}