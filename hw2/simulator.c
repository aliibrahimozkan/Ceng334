#include "helper.h"
#include "writeOutput.h"
#include <stdio.h>
#include <stdlib.h>
#include "pthread.h"

WaitCanDeposit(){

}

void sender_routine(int ID, int total_packages, int assigned_hub,int sleep_time){
  int current_hub =assigned_hub;
  int remaining_packages= total_packages;
  SenderInfo * sender_info = malloc(sizeof (struct SenderInfo));
  PackageInfo *package_info;

  FillSenderInfo(sender_info,ID,current_hub,total_packages,NULL);
  WriteOutput(sender_info,NULL,NULL,NULL,SENDER_CREATED);
  while (remaining_packages > 0){



    FillPacketInfo(packege_info,ID,current_hub,receiver_id,receiving_hub_id);
    FillSenderInfo(sender_info,ID,current_hub,remaining_packages,package_info);
    WriteOutput(sender_info,NULL,NULL,NULL,SENDER_DEPOSITED);

    remaining_packages--;
    wait(sleep_time * UNIT_TIME);
  }


}
void receiver_routine(int ID, int speed, int assigned_hub) {
  int current_hub = assigned_hub;
  ReceiverInfo * receiver_info = malloc(sizeof (struct ReceiverInfo));
  FillReceiverInfo(receiver_info,ID,current_hub,NULL);
  WriteOutput(NULL,receiver_info,NULL,NULL,RECEIVER_CREATED);




}













int main() {
  int hub_num;
  int *incoming_package_storage;
  int *outgoing_package_storage;
  int *number_of_charging_space;

  scanf("%d", &hub_num);
  incoming_package_storage = (int *)malloc(sizeof(int) * hub_num);
  outgoing_package_storage = (int *)malloc(sizeof(int) * hub_num);
  number_of_charging_space = (int *)malloc(sizeof(int) * hub_num);

  double **distances = (double **)malloc(hub_num * sizeof(double *));
  for (int i = 0; i < hub_num; i++) {
    distances[i] = (double *)malloc(hub_num * sizeof(double));
  }
  for (int i = 0; i < hub_num; i++) {
    scanf("%d", &incoming_package_storage[i]);
    scanf("%d", &outgoing_package_storage[i]);
    scanf("%d", &number_of_charging_space[i]);
    for (int j = 0; j < hub_num; j++) {
      scanf("%lf", &distances[i][j]);
    }
  }

  int *time_sender_waits_btw_packets = (int *)malloc(sizeof(int) * hub_num);
  int *hub_id_of_sender = (int *)malloc(sizeof(int) * hub_num);
  int *total_number_package_of_sender = (int *)malloc(sizeof(int) * hub_num);

  for (int i = 0; i < hub_num; i++) {
    scanf("%d", &time_sender_waits_btw_packets[i]);
    scanf("%d", &hub_id_of_sender[i]);
    scanf("%d", &total_number_package_of_sender[i]);
  }

  int *time_receiver_waits_btw_packets = (int *)malloc(sizeof(int) * hub_num);
  int *hub_id_of_receiver = (int *)malloc(sizeof(int) * hub_num);

  for (int i = 0; i < hub_num; i++) {
    scanf("%d", &time_receiver_waits_btw_packets[i]);
    scanf("%d", &hub_id_of_receiver[i]);
  }

  int num_drones;
  scanf("%d", &num_drones);

  int *speed_drone = (int *)malloc(sizeof(int) * num_drones);
  int *starting_hub_id = (int *)malloc(sizeof(int) * num_drones);
  int *maximum_range_of_drone = (int *)malloc(sizeof(int) * num_drones);

  for (int i = 0; i < num_drones; i++) {
    scanf("%d", &speed_drone[i]);
    scanf("%d", &starting_hub_id[i]);
    scanf("%d", &maximum_range_of_drone[i]);
  }

  free(incoming_package_storage);
  free(outgoing_package_storage);
  free(number_of_charging_space);
  for (int j = 0; j < hub_num; j++) {
    free(distances[j]);
  }

  free(distances);
  free(time_sender_waits_btw_packets);
  free(hub_id_of_sender);
  free(total_number_package_of_sender);
  free(time_receiver_waits_btw_packets);
  free(hub_id_of_receiver);
  free(speed_drone);
  free(starting_hub_id);
  free(maximum_range_of_drone);

  return 0;
}
