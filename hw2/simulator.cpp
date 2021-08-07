#include "helper.h"
#include "pthread.h"
#include "semaphore.h"
#include "writeOutput.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <sys/time.h>
#include <vector>

class droneClass {
public:
  droneClass() {}
  int hub_id;
  int id;
  int max_range;
  int curr_range;
  int speed;
  long long charging_time;
  timeval begin_charge;
  unsigned long long b_charge;
  unsigned long long e_charge;
  timeval end_charge;
  std::vector<PackageInfo> package;
  bool is_called;
  int destination_hub;
  void init_sem();
  sem_t package_mutex;
  sem_t called_mutex;
  sem_t is_called_mutex;
  sem_t package_semaphore;
  sem_t curr_range_mutex;
};

void droneClass::init_sem() {
  (void)sem_init(&is_called_mutex, 0, 1);
  (void)sem_init(&package_mutex, 0, 1);
  (void)sem_init(&curr_range_mutex, 0, 1);

  (void)sem_init(&called_mutex, 0, 0);
  (void)sem_init(&package_semaphore, 0, 1);
}

class receiverClass {
public:
  receiverClass() {}
  ~receiverClass() {}
  int time_between;
  int id;
  int hub_id;
};

class senderClass {
public:
  senderClass() {}
  ~senderClass() {}
  int time_between;
  int hub_id;
  int id;
  int number_of_package;
};

class hubClass {
public:
  hubClass(){};
  int n;
  int outgoing_storage;
  int id;
  int incoming_storage;
  int charging_space;
  std::vector<int> current_receivers;
  int drone_number;
  std::vector<PackageInfo> incoming_p_info;
  std::vector<PackageInfo> outgoing_p_info;
  int active_hub;
  bool is_deposited;
  std::vector<droneClass *> drone_in_hub;
  int *distance;
  int drone_number_read;
  sem_t out_str_mutex;
  sem_t in_str_mutex;
  sem_t is_deposited_mutex;
  sem_t incoming_str_current_semaphore;
  sem_t incoming_str_remain_semaphore;
  sem_t charging_current_semaphore;
  sem_t charging_remain_semaphore;
  sem_t outgoing_str_semaphore;
  sem_t in_deposited_semaphore;
  sem_t drone_number_semaphore;
  sem_t drone_number_write;
  sem_t drone_number_mutex;
  bool active;
  sem_t active_write;
  sem_t active_mutex;
  void set_distance(int *d);
  void init_sem();
  ~hubClass();
};

hubClass::~hubClass() { delete[] distance; }

void hubClass::init_sem() {
  (void)sem_init(&incoming_str_current_semaphore, 0, 0);
  (void)sem_init(&incoming_str_remain_semaphore, 0, incoming_storage);

  (void)sem_init(&outgoing_str_semaphore, 0, outgoing_storage);
  (void)sem_init(&out_str_mutex, 0, 1);
  (void)sem_init(&drone_number_semaphore, 0, drone_number);
  (void)sem_init(&is_deposited_mutex, 0, 1);

  (void)sem_init(&in_str_mutex, 0, 1);
  (void)sem_init(&in_deposited_semaphore, 0, 0);

  (void)sem_init(&charging_current_semaphore, 0, 0);
  (void)sem_init(&charging_remain_semaphore, 0, charging_space);

  (void)sem_init(&active_write, 0, 1);
  (void)sem_init(&active_mutex, 0, 1);

  (void)sem_init(&drone_number_write, 0, 1);
  (void)sem_init(&drone_number_mutex, 0, 1);
}

void hubClass::set_distance(int *d) {
  distance = new int[n];
  for (int i = 0; i < n; i++) {
    distance[i] = d[i];
  }
}

std::vector<senderClass *> sender_vector;
std::vector<receiverClass *> receiver_vector;
std::vector<hubClass *> hub_vector;
std::vector<droneClass *> drone_vector;

int active_hub;
int total_sender;
sem_t total_sender_mutex;
sem_t total_sender_write;
sem_t active_hub_write;
sem_t active_hub_mutex;
int total_sender_read;
int active_hub_read;

void *sender_routine(void *s) {
  senderClass *sender = (senderClass *)s;
  int current_hub = sender->hub_id;
  int remaining_packages = sender->number_of_package;
  int tb = sender->time_between;
  SenderInfo sender_info;
  PackageInfo package_info;
  FillSenderInfo(&sender_info, sender->id, current_hub, remaining_packages,
                 NULL);
  WriteOutput(&sender_info, NULL, NULL, NULL, SENDER_CREATED);
  int random_hub;
  int hub_no = ((*(hub_vector[sender->hub_id - 1])).n);
  int randomNumbers[hub_no - 1];
  int counter = 0;
  for (int i = 0; i < hub_no; i++) {
    if (i + 1 != sender->hub_id) {
      randomNumbers[counter] = i + 1;
      counter++;
    }
  }
  int ind;
  int random_rec;

  while (remaining_packages > 0) {

    ind = rand() % (hub_no - 1);
    random_hub = randomNumbers[ind];

    random_rec = hub_vector[random_hub - 1]->current_receivers.front();

    (void)sem_wait(
        &((*(hub_vector[sender->hub_id - 1])).outgoing_str_semaphore));

    package_info.sending_hub_id = sender->hub_id;
    package_info.receiving_hub_id = random_hub;
    package_info.receiver_id = random_rec;
    package_info.sender_id = sender->id;

    (void)sem_wait(&((*(hub_vector[sender->hub_id - 1])).out_str_mutex));

    (*(hub_vector[sender->hub_id - 1])).outgoing_p_info.push_back(package_info);

    (void)sem_post(&((*(hub_vector[sender->hub_id - 1])).out_str_mutex));

    FillPacketInfo(&package_info, sender->id, sender->hub_id, random_rec,
                   random_hub);
    FillSenderInfo(&sender_info, sender->id, sender->hub_id, remaining_packages,
                   &package_info);
    WriteOutput(&sender_info, NULL, NULL, NULL, SENDER_DEPOSITED);
    remaining_packages--;
    wait(tb * UNIT_TIME);
  }
  (void)sem_wait(&total_sender_write);
  total_sender -= 1;
  (void)sem_post(&total_sender_write);

  FillSenderInfo(&sender_info, sender->id, current_hub, remaining_packages,
                 NULL);
  WriteOutput(&sender_info, NULL, NULL, NULL, SENDER_STOPPED);
  return 0;
}

void *receiver_routine(void *r) {
  receiverClass *receiver = (receiverClass *)r;
  ReceiverInfo receiver_info;
  PackageInfo package_info;
  std::vector<int> package_sender_id;
  std::vector<int> package_sender_hub_id;
  int current_hub = receiver->hub_id;
  bool is_active;
  FillReceiverInfo(&receiver_info, receiver->id, current_hub, NULL);
  WriteOutput(NULL, &receiver_info, NULL, NULL, RECEIVER_CREATED);
  while (true) {
    int ss;
    sem_post(&hub_vector[current_hub - 1]->active_mutex);
    is_active = hub_vector[current_hub - 1]->active;
    sem_wait(&hub_vector[current_hub - 1]->active_mutex);

    (void)sem_wait(&(hub_vector[current_hub - 1]->in_str_mutex));
    ss = hub_vector[current_hub - 1]->incoming_p_info.size();
    (void)sem_post(&((*(hub_vector[current_hub - 1])).in_str_mutex));

    if ((!is_active) && (ss == 0)) {
      break;
    }

    if (ss > 0) {

      (void)sem_wait(&(hub_vector[current_hub - 1]->in_str_mutex));

      for (unsigned int i = 0;
           i < hub_vector[current_hub - 1]->incoming_p_info.size(); i++) {
        package_sender_id.push_back(
            hub_vector[current_hub - 1]->incoming_p_info[i].sender_id);
        package_sender_hub_id.push_back(
            hub_vector[current_hub - 1]->incoming_p_info[i].sending_hub_id);
      }

      hub_vector[current_hub - 1]->incoming_p_info.clear();

      (void)sem_post(&((*(hub_vector[current_hub - 1])).in_str_mutex));

      for (unsigned int i = 0; i < package_sender_hub_id.size(); i++) {
        FillPacketInfo(&package_info, package_sender_id[i],
                       package_sender_hub_id[i], receiver->id,
                       receiver->hub_id);
        FillReceiverInfo(&receiver_info, receiver->id, receiver->hub_id,
                         &package_info);
        WriteOutput(NULL, &receiver_info, NULL, NULL, RECEIVER_PICKUP);
        (void)sem_post(
            &(*(hub_vector[current_hub - 1])).incoming_str_remain_semaphore);

        wait(UNIT_TIME * receiver->time_between);
      }
      package_sender_hub_id.clear();
      package_sender_id.clear();
    }
  }
  FillReceiverInfo(&receiver_info, receiver->id, current_hub, NULL);
  WriteOutput(NULL, &receiver_info, NULL, NULL, RECEIVER_STOPPED);
  return 0;
}

void *drone_routine(void *d) {
  droneClass *drone = (droneClass *)d;
  int current_hub = drone->hub_id;
  int current_range = drone->curr_range;
  DroneInfo drone_info;
  PackageInfo package_info;
  FillDroneInfo(&drone_info, drone->id, drone->hub_id, current_range, NULL, 0);
  WriteOutput(NULL, NULL, &drone_info, NULL, DRONE_CREATED);
  int p_sender_id;
  int p_sender_hub_id;
  int p_receiver_hub_id;
  int p_receiver_id;
  int d_current_range;
  int ah;
  long long charge_time;
  int temp;
  drone->b_charge = GetTimestamp();
  while (true) {

    sem_wait(&active_hub_mutex);
    active_hub_read++;
    if (total_sender_read == 1) {
      sem_wait(&active_hub_write);
    }
    sem_post(&active_hub_mutex);
    // read
    ah = active_hub;
    sem_wait(&active_hub_mutex);
    active_hub_read--;
    if (active_hub_read == 0) {
      sem_post(&active_hub_write);
    }
    sem_post(&active_hub_mutex);
    int val;
    (void)sem_wait(&drone->package_mutex);
    val = drone->package.size();
    (void)sem_post(&drone->package_mutex);

    if (ah == 0 && val == 0) {
      break;
    }

    sem_wait(&drone->is_called_mutex);
    bool is_called = drone->is_called;
    sem_post(&drone->is_called_mutex);

    if (is_called) {
      current_hub = drone->hub_id;

      sem_wait(&drone->curr_range_mutex);
      d_current_range = drone->curr_range;
      sem_post(&drone->curr_range_mutex);

      (void)sem_wait(&drone->package_mutex);
      val = drone->package.size();
      (void)sem_post(&drone->package_mutex);

      if (val != 0) {
        (void)sem_wait(&(hub_vector[drone->package.front().receiving_hub_id - 1]
                             ->incoming_str_remain_semaphore));
        (void)sem_wait(&(hub_vector[drone->package.front().receiving_hub_id - 1]
                             ->charging_remain_semaphore));

        (void)sem_wait(&(drone->package_mutex));
        p_sender_id = drone->package.front().sender_id;
        p_sender_hub_id = drone->package.front().sending_hub_id;
        p_receiver_hub_id = drone->package.front().receiving_hub_id;
        p_receiver_id = drone->package.front().receiver_id;

        (void)sem_post(&(drone->package_mutex));

        int distance =
            hub_vector[p_sender_hub_id - 1]->distance[p_receiver_hub_id - 1];
        int required_charge = distance / (drone->speed);

        drone->e_charge = GetTimestamp();
        charge_time = drone->e_charge - drone->b_charge;
        d_current_range = calculate_drone_charge(
            charge_time, d_current_range, drone->max_range);
        temp = d_current_range;

        while (temp < required_charge) {
          drone->e_charge = GetTimestamp();
          charge_time = drone->e_charge - drone->b_charge;
            temp  = calculate_drone_charge(
              charge_time, d_current_range, drone->max_range);
        }
        d_current_range = temp;
        drone->charging_time = charge_time;


        FillPacketInfo(&package_info, p_sender_id, p_sender_hub_id,
                       p_receiver_id, p_receiver_hub_id);

        FillDroneInfo(&drone_info, drone->id, drone->hub_id, d_current_range,
                      &package_info, 0);
        WriteOutput(NULL, NULL, &drone_info, NULL, DRONE_PICKUP);

        sem_wait(&hub_vector[current_hub - 1]->is_deposited_mutex);
        hub_vector[current_hub - 1]->is_deposited = true;
        sem_post(&hub_vector[current_hub - 1]->is_deposited_mutex);

        (void)sem_post(
            &(*(hub_vector[drone->hub_id - 1])).outgoing_str_semaphore);

        (void)sem_post(
            &(*(hub_vector[drone->hub_id - 1])).charging_remain_semaphore);

        sem_wait(&hub_vector[current_hub - 1]->drone_number_write);
        hub_vector[current_hub - 1]->drone_number--;

        hub_vector[drone->hub_id - 1]->drone_in_hub.erase(
            std::remove(hub_vector[drone->hub_id - 1]->drone_in_hub.begin(),
                        hub_vector[drone->hub_id - 1]->drone_in_hub.end(),
                        drone),
            hub_vector[drone->hub_id - 1]->drone_in_hub.end());

        sem_post(&hub_vector[current_hub - 1]->drone_number_write);

        travel(distance, drone->speed);
        //std::cout <<distance <<"=="<< drone->speed <<std::endl;
        d_current_range =
            d_current_range - range_decrease(distance, drone->speed);

        sem_wait(&drone->curr_range_mutex);
        drone->curr_range = d_current_range;
        sem_post(&drone->curr_range_mutex);

        drone->hub_id = drone->package.front().receiving_hub_id;
        current_hub = drone->hub_id;

        (void)sem_wait(&(hub_vector[current_hub - 1]->in_str_mutex));

        hub_vector[current_hub - 1]->incoming_p_info.push_back(
            drone->package.front());

        (void)sem_post(&(hub_vector[current_hub - 1]->in_str_mutex));
        (void)sem_post(&(hub_vector[current_hub - 1]->in_deposited_semaphore));

        drone->b_charge = GetTimestamp();

        FillPacketInfo(&package_info, p_sender_id, p_sender_hub_id,
                       p_receiver_id, p_receiver_hub_id);
        FillDroneInfo(&drone_info, drone->id, drone->hub_id, d_current_range,
                      &package_info, 0);
        WriteOutput(NULL, NULL, &drone_info, NULL, DRONE_DEPOSITED);
        (void)sem_wait(&(drone->package_mutex));
        drone->package.erase(drone->package.begin());
        (void)sem_post(&(drone->package_mutex));

        sem_wait(&drone->is_called_mutex);
        drone->is_called = false;
        sem_post(&drone->is_called_mutex);

        sem_wait(&(hub_vector[current_hub - 1]->drone_number_write));
        hub_vector[current_hub - 1]->drone_number++;
        hub_vector[current_hub - 1]->drone_in_hub.push_back(drone);
        sem_post(&(hub_vector[current_hub - 1]->drone_number_write));

        (void)sem_post(&(drone->package_semaphore));

      } else {

        (void)sem_wait(&(*(hub_vector[drone->destination_hub - 1]))
                            .charging_remain_semaphore);
        int distance =
            hub_vector[drone->hub_id - 1]->distance[drone->destination_hub - 1];
        int required_charge = distance / drone->speed;
        drone->e_charge = GetTimestamp();
        charge_time = drone->e_charge - drone->b_charge;

        d_current_range = calculate_drone_charge(
            charge_time, d_current_range, drone->max_range);
        temp = d_current_range;

        while (temp < required_charge) {
          drone->e_charge = GetTimestamp();
          charge_time = drone->e_charge - drone->b_charge;
          temp  = calculate_drone_charge(
              charge_time, d_current_range, drone->max_range);
        }

        d_current_range = temp;
        drone->charging_time = charge_time;

        FillDroneInfo(&drone_info, drone->id, drone->hub_id, d_current_range,
                      NULL, drone->destination_hub);
        WriteOutput(NULL, NULL, &drone_info, NULL, DRONE_GOING);
        (void)sem_post(
            &(*(hub_vector[drone->hub_id - 1])).charging_remain_semaphore);

        sem_wait(&hub_vector[current_hub - 1]->drone_number_write);
        hub_vector[current_hub - 1]->drone_number--;
        hub_vector[drone->hub_id - 1]->drone_in_hub.erase(
            std::remove(hub_vector[drone->hub_id - 1]->drone_in_hub.begin(),
                        hub_vector[drone->hub_id - 1]->drone_in_hub.end(),
                        drone),
            hub_vector[drone->hub_id - 1]->drone_in_hub.end());
        sem_post(&hub_vector[current_hub - 1]->drone_number_write);

        travel(distance, drone->speed);

        d_current_range =
            d_current_range - range_decrease(distance, drone->speed);
        drone->hub_id = drone->destination_hub;

        sem_wait(&drone->curr_range_mutex);
        drone->curr_range = d_current_range;
        sem_post(&drone->curr_range_mutex);
        drone->b_charge = GetTimestamp();
        current_hub = drone->hub_id;
        FillDroneInfo(&drone_info, drone->id, drone->hub_id, d_current_range,
                      NULL, 0);
        WriteOutput(NULL, NULL, &drone_info, NULL, DRONE_ARRIVED);

        sem_wait(&drone->is_called_mutex);
        drone->is_called = false;
        sem_post(&drone->is_called_mutex);

        sem_wait(&hub_vector[current_hub - 1]->drone_number_write);
        hub_vector[current_hub - 1]->drone_number++;
        hub_vector[current_hub - 1]->drone_in_hub.push_back(drone);
        sem_post(&hub_vector[current_hub - 1]->drone_number_write);
      }
    }
  }

  FillDroneInfo(&drone_info, drone->id, drone->hub_id, drone->curr_range, NULL,
                0);
  WriteOutput(NULL, NULL, &drone_info, NULL, DRONE_STOPPED);
  return 0;
}

void *hub_routine(void *h) {
  hubClass *hub = (hubClass *)h;
  HubInfo hub_info;
  FillHubInfo(&hub_info, hub->id);
  WriteOutput(NULL, NULL, NULL, &hub_info, HUB_CREATED);
  int sender_number;
  bool is_dep;
  while (true) {
    sem_wait(&total_sender_mutex);
    total_sender_read++;
    if (total_sender_read == 1) {
      sem_wait(&total_sender_write);
    }
    sem_post(&total_sender_mutex);
    // read
    sender_number = total_sender;
    sem_wait(&total_sender_mutex);
    total_sender_read--;
    if (total_sender_read == 0) {
      sem_post(&total_sender_write);
    }
    sem_post(&total_sender_mutex);

    int out_size;
    sem_wait(&(hub->out_str_mutex));
    out_size = hub->outgoing_p_info.size();
    sem_post(&(hub->out_str_mutex));
    int in_size;

    sem_wait(&(hub->in_str_mutex));
    in_size = hub->incoming_p_info.size();
    sem_post(&(hub->in_str_mutex));

    sem_wait(&(hub->is_deposited_mutex));
    is_dep = hub->is_deposited;
    sem_post(&(hub->is_deposited_mutex));

    if (sender_number == 0 && out_size == 0 && in_size == 0 && is_dep) {
      break;
    }

    if (out_size > 0) {
      int size;
      sem_wait(&hub->drone_number_mutex);
      hub->drone_number_read++;
      if (hub->drone_number_read == 1) {
        sem_wait(&hub->drone_number_write);
      }
      sem_post(&hub->drone_number_mutex);
      // read
      size = hub->drone_number;
      if (size != 0) {
        std::vector<std::pair<int, int>> sorted_drone;

        for (int i = 0; i < size; i++) {
          sem_wait(&(hub->drone_in_hub[i]->curr_range_mutex));
          sorted_drone.push_back(
              std::make_pair(hub->drone_in_hub[i]->curr_range, i));
          sem_post(&(hub->drone_in_hub[i]->curr_range_mutex));
        }

        std::sort(sorted_drone.begin(), sorted_drone.end());
        for (int i = 0; i < size; i++) {
          sem_wait(&hub->drone_in_hub[sorted_drone[i].second]->is_called_mutex);

          if (!(hub->drone_in_hub[sorted_drone[i].second]->is_called)) {
            (void)sem_wait(&(
                hub->drone_in_hub[sorted_drone[i].second]->package_semaphore));

            (void)sem_wait(
                &hub->drone_in_hub[sorted_drone[i].second]->package_mutex);

            (void)sem_wait(&(hub->out_str_mutex));

            hub->drone_in_hub[sorted_drone[i].second]->package.push_back(
                *(hub->outgoing_p_info.begin()));

            hub->outgoing_p_info.erase(hub->outgoing_p_info.begin());

            (void)sem_post(&(hub->out_str_mutex));

            sem_wait(&(hub->is_deposited_mutex));
            hub->is_deposited = false;
            sem_post(&(hub->is_deposited_mutex));

            (void)sem_post(
                &hub->drone_in_hub[sorted_drone[i].second]->package_mutex);
            (void)sem_post(
                &(hub->drone_in_hub[sorted_drone[i].second]->called_mutex));

            hub->drone_in_hub[sorted_drone[i].second]->is_called = true;

            sem_post(
                &hub->drone_in_hub[sorted_drone[i].second]->is_called_mutex);

            break;
          }
          sem_post(&hub->drone_in_hub[sorted_drone[i].second]->is_called_mutex);
        }

        sem_wait(&hub->drone_number_mutex);
        hub->drone_number_read--;
        if (hub->drone_number_read == 0) {
          sem_post(&hub->drone_number_write);
        }
        sem_post(&hub->drone_number_mutex);

      } else {

        sem_wait(&hub->drone_number_mutex);
        hub->drone_number_read--;
        if (hub->drone_number_read == 0) {
          sem_post(&hub->drone_number_write);
        }
        sem_post(&hub->drone_number_mutex);

        std::vector<std::pair<int, int>> sorted_distance;
        for (int i = 0; i < hub->n; i++) {
          if (i != hub->id - 1) {
            sorted_distance.push_back(std::make_pair(hub->distance[i], i));
          }
        }
        std::sort(sorted_distance.begin(), sorted_distance.end());
        int val2;

        bool d_no = false;
        for (int i = 0; i < hub->n - 1; i++) {

          sem_wait(&hub_vector[sorted_distance[i].second]->drone_number_mutex);
          hub_vector[sorted_distance[i].second]->drone_number_read++;
          if (hub_vector[sorted_distance[i].second]->drone_number_read == 1) {
            sem_wait(
                &hub_vector[sorted_distance[i].second]->drone_number_write);
          }
          sem_post(&hub_vector[sorted_distance[i].second]->drone_number_mutex);
          // read
          val2 = hub_vector[sorted_distance[i].second]->drone_number;
          if (val2 != 0) {
            for (int j = 0; j < val2; j++) {
              sem_wait(&hub_vector[sorted_distance[i].second]
                            ->drone_in_hub[j]
                            ->is_called_mutex);
              if (!hub_vector[sorted_distance[i].second]
                       ->drone_in_hub[j]
                       ->is_called) {
                (void)sem_post(&(hub_vector[sorted_distance[i].second]
                                     ->drone_in_hub[j]
                                     ->called_mutex));
                hub_vector[sorted_distance[i].second]
                    ->drone_in_hub[j]
                    ->destination_hub = hub->id;
                hub_vector[sorted_distance[i].second]
                    ->drone_in_hub[j]
                    ->is_called = true;
                d_no = true;
              }
              sem_post(&hub_vector[sorted_distance[i].second]
                            ->drone_in_hub[j]
                            ->is_called_mutex);
              if (d_no) {
                break;
              }
            }
          }
          sem_wait(&hub_vector[sorted_distance[i].second]->drone_number_mutex);
          hub_vector[sorted_distance[i].second]->drone_number_read--;
          if (hub_vector[sorted_distance[i].second]->drone_number_read == 0) {
            sem_post(
                &hub_vector[sorted_distance[i].second]->drone_number_write);
          }
          sem_post(&hub_vector[sorted_distance[i].second]->drone_number_mutex);
          if (d_no) {
            break;
          }
        }

        if (!d_no) {
          wait(UNIT_TIME);
        } else {

          while (true) {
            int size2;
            sem_wait(&hub->drone_number_mutex);
            hub->drone_number_read++;
            if (hub->drone_number_read == 1) {
              sem_wait(&hub->drone_number_write);
            }
            sem_post(&hub->drone_number_mutex);
            // read
            size2 = hub->drone_number;
            sem_wait(&hub->drone_number_mutex);
            hub->drone_number_read--;
            if (hub->drone_number_read == 0) {
              sem_post(&hub->drone_number_write);
            }
            sem_post(&hub->drone_number_mutex);
            if (size2 != 0) {
              break;
            }
          }
        }
      }
    }
  }

  (void)sem_wait(&active_hub_write);
  active_hub -= 1;
  (void)sem_post(&active_hub_write);

  (void)sem_wait(&hub->active_mutex);
  hub->active = false;
  (void)sem_post(&hub->active_mutex);

  FillHubInfo(&hub_info, hub->id);
  WriteOutput(NULL, NULL, NULL, &hub_info, HUB_STOPPED);
  return 0;
}

int main() {
  int hub_num;
  int ips;
  int ops;
  int cs;
  std::cin >> hub_num;
  int *d = new int[hub_num];

  hubClass *hub = new hubClass[hub_num]();
  senderClass *sender = new senderClass[hub_num]();
  active_hub = hub_num;
  total_sender = hub_num;
  (void)sem_init(&total_sender_mutex, 0, 1);
  (void)sem_init(&active_hub_mutex, 0, 1);
  (void)sem_init(&active_hub_write, 0, 1);
  (void)sem_init(&total_sender_write, 0, 1);
  total_sender_read = 0;
  active_hub_read = 0;
  for (int i = 0; i < hub_num; i++) {
    std::cin >> ips;
    std::cin >> ops;
    std::cin >> cs;
    for (int j = 0; j < hub_num; j++) {
      std::cin >> d[j];
    }
    hub[i].id = i + 1;
    hub[i].n = hub_num;
    hub[i].incoming_storage = ips;
    hub[i].outgoing_storage = ops;
    hub[i].charging_space = cs;
    hub[i].set_distance(d);
    hub[i].drone_number = 0;
    hub[i].is_deposited = true;
    hub[i].drone_number_read = 0;
    hub[i].active_hub = hub_num;
    hub[i].active = true;
    hub_vector.push_back(&(hub[i]));
  }

  int ts;
  int hid;
  int tp;

  for (int i = 0; i < hub_num; i++) {
    std::cin >> ts;
    std::cin >> hid;
    std::cin >> tp;
    sender[i].hub_id = hid;
    sender[i].number_of_package = tp;
    sender[i].time_between = ts;
    sender[i].id = i + 1;

    sender_vector.push_back(&(sender[i]));
  }

  int tr;
  receiverClass *receiver = new receiverClass[hub_num]();
  for (int i = 0; i < hub_num; i++) {
    std::cin >> tr;
    std::cin >> hid;
    receiver[i].hub_id = hid;
    receiver[i].time_between = tr;
    receiver[i].id = i + 1;
    hub[hid - 1].current_receivers.push_back(i + 1);

    receiver_vector.push_back(&(receiver[i]));
  }

  int num_drones;
  std::cin >> num_drones;
  droneClass *drone = new droneClass[num_drones]();
  int s;
  int mr;
  for (int i = 0; i < num_drones; i++) {
    std::cin >> s;
    std::cin >> hid;
    std::cin >> mr;
    drone[i].hub_id = hid;
    hub[hid - 1].drone_number += 1;
    drone[i].init_sem();
    drone[i].speed = s;
    drone[i].max_range = mr;
    drone[i].curr_range = mr;
    drone[i].id = i + 1;
    gettimeofday(&drone[i].end_charge, NULL);
    gettimeofday(&drone[i].begin_charge, NULL);
    drone[i].charging_time = 0;
    drone[i].destination_hub = 0;
    drone[i].is_called = false;
    hub[hid - 1].drone_in_hub.push_back(&(drone[i]));
    drone_vector.push_back(&(drone[i]));
  }

  for (int i = 0; i < hub_num; i++) {
    hub[i].init_sem();
  }
  pthread_t *sender_t = new pthread_t[hub_num];
  pthread_t *receiver_t = new pthread_t[hub_num];
  pthread_t *drone_t = new pthread_t[num_drones];
  pthread_t *hub_t = new pthread_t[hub_num];

  InitWriteOutput();
  for (int i = 0; i < hub_num; i++) {
    (void)pthread_create(&hub_t[i], NULL, hub_routine, &hub[i]);
  }

  for (int i = 0; i < num_drones; i++) {
    (void)pthread_create(&drone_t[i], NULL, drone_routine, &drone[i]);
  }

  for (int i = 0; i < hub_num; i++) {
    (void)pthread_create(&receiver_t[i], NULL, receiver_routine, &receiver[i]);
  }

  for (int i = 0; i < hub_num; i++) {
    (void)pthread_create(&sender_t[i], NULL, sender_routine, &sender[i]);
  }
  for (int i = 0; i < hub_num; i++) {
    (void)pthread_join(sender_t[i], NULL);
  }
  for (int i = 0; i < hub_num; i++) {
    (void)pthread_join(hub_t[i], NULL);
  }
  for (int i = 0; i < num_drones; i++) {
    (void)pthread_join(drone_t[i], NULL);
  }

  for (int i = 0; i < hub_num; i++) {
    (void)pthread_join(receiver_t[i], NULL);
  }

  delete[] sender_t;
  delete[] receiver_t;
  delete[] drone_t;
  delete[] hub_t;
  delete[] hub;
  delete[] sender;
  delete[] receiver;
  delete[] drone;
  delete[] d;

  return 0;
}
