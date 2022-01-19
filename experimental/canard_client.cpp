/*
 * canard_client.cpp
 * Created on: Jan 17, 2022 22:09
 * Description:
 *
 * Copyright (c) 2022 Pin Loon Lee (pllee4)
 */

#include <unistd.h>

#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>

#include "canard.h"
#include "socketcan.h"

void *MemoryAllocate(CanardInstance *ins, size_t amount) {
  return malloc(amount * (sizeof(uint8_t)));
}
void MemoryFree(CanardInstance *ins, void *pointer) { free(pointer); }

void Client(CanardInstance *inst) {
  const char *const iface_name = "vcan0";
  const auto sb = socketcanOpen(iface_name, false);
  if (sb < 0) {
    std::cout << "failed to open can: " << iface_name << std::endl;
  }
  // setup receive
  CanardRxSubscription test_subscription;
  (void)canardRxSubscribe(inst,  // Subscribe to messages uavcan.node.Heartbeat.
                          CanardTransferKindResponse,
                          100,   // The fixed Subject-ID of the Heartbeat
                                 // message type (see DSDL definition).
                          1024,  // The extent (the maximum possible payload
                                 // size); pick a huge value if not sure.
                          CANARD_DEFAULT_TRANSFER_ID_TIMEOUT_USEC,
                          &test_subscription);

  std::cout << "client thread started" << std::endl;

  char buf[4096]{};
  CanardFrame rcv_frame{};

  while (true) {
    const CanardFrame *send_frame = canardTxPeek(inst);
    if (send_frame != NULL) {
      socketcanPush(sb, send_frame, 1'000'000);
      canardTxPop(inst);
      inst->memory_free(inst, (CanardFrame *)send_frame);
    }

    if (socketcanPop(sb, &rcv_frame, sizeof(buf), buf, 0, nullptr) == 1) {
      static CanardTransfer transfer;
      const int8_t result = canardRxAccept(inst, &rcv_frame, 0, &transfer);
      if (result == 1) {
        if ((transfer.transfer_kind == CanardTransferKindResponse) &&
            (transfer.port_id == 100)) {
          std::cout << "Received response" << std::endl;
          uint8_t *data =
              static_cast<uint8_t *>(const_cast<void *>(transfer.payload));
          for (uint8_t i = 0; i < 10; ++i) {
            printf("Payload[%d]: %d\n", i, data[i]);
          }
          printf("\n");
        }
        inst->memory_free(inst, (void *)transfer.payload);
      }
    }
  }
}

int main(int argc, char *argv[]) {
  CanardInstance inst = canardInit(MemoryAllocate, MemoryFree);
  inst.mtu_bytes = CANARD_MTU_CAN_CLASSIC;
  inst.node_id = 43;
  std::thread client_thread(Client, &inst);

  uint8_t buffer[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
  size_t buffer_size = 1024;

  // setup transfer
  CanardTransfer transfer;
  transfer.priority = CanardPriorityNominal;
  transfer.transfer_kind = CanardTransferKindRequest;
  transfer.port_id = 100;
  transfer.remote_node_id = 42;
  transfer.transfer_id = 0;
  transfer.payload_size = 0;
  transfer.payload = NULL;

  uint8_t timer_count = 0;
  while (true)
  {
    if (timer_count++ == 0) {
      transfer.transfer_id++;
      canardTxPush(&inst, &transfer);
    }
    usleep(10000);
  }

  return 0;
}