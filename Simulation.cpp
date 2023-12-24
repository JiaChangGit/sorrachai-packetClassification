#include "Simulation.h"

#include <sstream>
#include <string>

std::mt19937 Random::generator(0);

std::vector<Request> Simulator::GenerateRequests(int num_packet, int num_insert,
                                                 int num_delete) const {
  if (num_packet > packets.size()) {
    printf(
        "Warning in Simulator::GenerateRequests : too much request for "
        "num_packet--setting num to available size\n");
    num_packet = packets.size();
  }
  std::vector<Request> vr;
  for (int i = 0; i < num_packet; i++)
    vr.emplace_back(Request(RequestType::ClassifyPacket));
  for (int i = 0; i < num_insert; i++)
    vr.emplace_back(Request(RequestType::Insertion));
  for (int i = 0; i < num_delete; i++)
    vr.emplace_back(Request(RequestType::Deletion));
  return Random::shuffle_vector(vr);
}

std::vector<Request> Simulator::SetupComputation(int num_packet, int num_insert,
                                                 int num_delete) {
  std::vector<Request> sequence =
      GenerateRequests(num_packet, num_insert, num_delete);
  std::vector<Rule> shuff_rules = ruleset;
  shuff_rules = Random::shuffle_vector(shuff_rules);
  Bookkeeper rules_in_use_temp =
      (std::vector<Rule>(std::begin(shuff_rules),
                         std::begin(shuff_rules) + shuff_rules.size() / 2));
  Bookkeeper available_pool_temp = (std::vector<Rule>(
      std::begin(shuff_rules) + shuff_rules.size() / 2, std::end(shuff_rules)));

  // need to adjust the case hitting treshold where there is nothing to delete
  // or elements are full but trying to add
  int current_size = rules_in_use_temp.size();
  double treshold = 0.2;
  for (size_t i = 0; i < sequence.size(); i++) {
    Request n = sequence[i];
    Rule temp_rule;
    switch (n.request_type) {
      case RequestType::ClassifyPacket:
        break;
      case RequestType::Insertion:
        if (current_size >= (1 - treshold) * ruleset.size()) {
          /// find furthest deletion and swap
          for (size_t j = sequence.size() - 1; j > i; j--) {
            if (sequence[j].request_type == RequestType::Deletion) {
              sequence[i].request_type = RequestType::Deletion;
              sequence[j].request_type = RequestType::Insertion;
              current_size--;
              break;
            }
          }
          // current_size++;
          // sequence[i].request_type = RequestType::Deletion;
          break;
        }
        current_size++;
        break;
      case RequestType::Deletion:
        if (current_size <= treshold * ruleset.size()) {
          /// find furthest insertion and swap
          for (size_t j = sequence.size() - 1; j > i; j--) {
            if (sequence[j].request_type == RequestType::Insertion) {
              sequence[j].request_type = RequestType::Deletion;
              sequence[i].request_type = RequestType::Insertion;
              current_size++;
              break;
            }
          }
          //	current_size--;
          //	sequence[i].request_type = RequestType::Insertion;
          break;
        }
        current_size--;
        break;
      default:
        break;
    }
  }

  for (Request& n : sequence) {
    Rule temp_rule;
    int result = -1;
    int index_delete = -1;
    int index_insert = -1;
    switch (n.request_type) {
      case RequestType::ClassifyPacket:
        break;
      case RequestType::Insertion:
        if (current_size >= (1 - treshold) * ruleset.size()) {
          printf("Warning skipped perform insertion: no available pool\n");
          break;
        }
        index_insert = Random::random_int(0, available_pool_temp.size() - 1);
        temp_rule = available_pool_temp.GetOneRuleAndPop(index_insert);
        rules_in_use_temp.InsertRule(temp_rule);
        n.random_index_trace = index_insert;
        break;
      case RequestType::Deletion:
        if (current_size <= treshold * ruleset.size()) {
          printf(
              "Warning skipped perform deletion: no avilable rules_in_use\n");
          break;
        }
        index_delete = Random::random_int(0, rules_in_use_temp.size() - 1);
        temp_rule = rules_in_use_temp.GetOneRuleAndPop(index_delete);
        available_pool_temp.InsertRule(temp_rule);
        n.random_index_trace = index_delete;
        break;
      default:
        break;
    }
  }

  rules_in_use =
      (std::vector<Rule>(std::begin(shuff_rules),
                         std::begin(shuff_rules) + shuff_rules.size() / 2));
  available_pool = (std::vector<Rule>(
      std::begin(shuff_rules) + shuff_rules.size() / 2, std::end(shuff_rules)));

  return sequence;
}
std::vector<int> Simulator::PerformOnlyPacketClassification(
    PacketClassifier& classifier,
    std::map<std::string, std::string>& summary) const {
  std::chrono::time_point<std::chrono::steady_clock> start, end;
  std::chrono::duration<double> elapsed_seconds;
  std::chrono::duration<double, std::milli> elapsed_milliseconds;

  start = std::chrono::steady_clock::now();
  classifier.ConstructClassifier(ruleset);
  end = std::chrono::steady_clock::now();
  elapsed_milliseconds = end - start;
  printf("\tConstruction time: %f ms\n", elapsed_milliseconds.count());
  summary["ConstructionTime(ms)"] =
      std::to_string(elapsed_milliseconds.count());

  const int trials = 10;
  std::chrono::duration<double> sum_time(0);
  std::vector<int> results;
  for (int t = 0; t < trials; t++) {
    results.clear();
    start = std::chrono::steady_clock::now();
    for (auto const& p : packets) {
      results.emplace_back(classifier.ClassifyAPacket(p));
    }
    end = std::chrono::steady_clock::now();
    elapsed_seconds = end - start;
    sum_time += elapsed_seconds;
  }

  printf("\tClassification time: %f s\n", sum_time.count() / trials);
  summary["ClassificationTime(s)"] = std::to_string(sum_time.count() / trials);

  int memSize = classifier.MemSizeBytes();
  printf("\tSize(bytes): %d \n", memSize);
  summary["Size(bytes)"] = std::to_string(memSize);
  int memAccess = classifier.MemoryAccess();
  printf("\tMemoryAccess: %d \n", memAccess);
  summary["MemoryAccess"] = std::to_string(memAccess);
  int numTables = classifier.NumTables();
  printf("\tTables: %d \n", numTables);
  summary["Tables"] = std::to_string(numTables);

  std::stringstream ssTableSize, ssTableQuery;
  for (int i = 0; i < numTables; i++) {
    if (i != 0) {
      ssTableSize << "-";
      ssTableQuery << "-";
    }
    ssTableSize << classifier.RulesInTable(i);
    ssTableQuery << classifier.NumPacketsQueriedNTables(i + 1);
  }
  summary["TableSizes"] = ssTableSize.str();
  summary["TableQueries"] = ssTableQuery.str();

  printf("\tTotal tables queried: %d\n", classifier.TablesQueried());
  printf("\tAverage tables queried: %f\n",
         1.0 * classifier.TablesQueried() / (trials * packets.size()));
  summary["AvgQueries"] = std::to_string(1.0 * classifier.TablesQueried() /
                                         (trials * packets.size()));

  return results;
}
std::vector<int> Simulator::PerformPacketClassification(
    PacketClassifier& classifier, const std::vector<Request>& sequence,
    std::map<std::string, double>& trial) const {
  if (available_pool.size() == 0) {
    printf(
        "Warning no avilable pool left: need to generate computation first\n");
    return std::vector<int>();
  }

  Bookkeeper rules_in_use_temp = rules_in_use;
  Bookkeeper available_pool_temp = available_pool;

  std::chrono::time_point<std::chrono::steady_clock> start, end;
  std::chrono::duration<double> elapsed_seconds;
  start = std::chrono::steady_clock::now();
  classifier.ConstructClassifier(rules_in_use_temp.GetRules());
  end = std::chrono::steady_clock::now();
  elapsed_seconds = end - start;
  // printf("Construction time: %f \n", elapsed_seconds.count());

  int num_trial = 10;
  std::vector<int> results;
  std::chrono::duration<double> sum_elapsed2(0);
  for (int t = 0; t < num_trial; t++) {
    results.clear();
    results.reserve(1000000);  // reserve 1m slots for packets

    int packet_counter = 0;
    // invariant: at all time, DS.rules = rules_in_use.rules
    std::chrono::duration<double> elapsed_seconds2(0);
    for (Request n : sequence) {
      Rule temp_rule;
      int result = -1;
      switch (n.request_type) {
        case RequestType::ClassifyPacket:
          /*if (packets.size() == 0) {
                  printf("Warning packets.size() = 0 in packet request");
                  break;
                  }*/
          start = std::chrono::steady_clock::now();
          result = classifier.ClassifyAPacket(packets[packet_counter++]);
          end = std::chrono::steady_clock::now();
          elapsed_seconds2 += end - start;
          if (packet_counter == packets.size()) packet_counter = 0;

          results.emplace_back(result);
          break;
        case RequestType::Insertion:
          if (available_pool.size() == 0) {
            printf("Warning skipped perform insertion: no available pool\n");
            break;
          }
          temp_rule =
              available_pool_temp.GetOneRuleAndPop(n.random_index_trace);
          rules_in_use_temp.InsertRule(temp_rule);
          start = std::chrono::steady_clock::now();
          classifier.InsertRule(temp_rule);
          end = std::chrono::steady_clock::now();
          elapsed_seconds2 += end - start;

          break;
        case RequestType::Deletion:
          if (rules_in_use.size() == 0) {
            printf(
                "Warning skipped perform deletion: no avilable rules_in_use\n");
            break;
          }
          temp_rule = rules_in_use_temp.GetOneRuleAndPop(n.random_index_trace);
          available_pool_temp.InsertRule(temp_rule);

          start = std::chrono::steady_clock::now();
          classifier.DeleteRule(n.random_index_trace);
          end = std::chrono::steady_clock::now();
          elapsed_seconds2 += end - start;

          break;
        default:
          break;
      }
    }
    sum_elapsed2 += elapsed_seconds2;
  }

  printf("\tUpdateTime time: %f \n", sum_elapsed2.count() / num_trial);
  if (!trial.count("UpdateTime(s)")) {
    trial["UpdateTime(s)"] = 0;
  }
  trial["UpdateTime(s)"] += sum_elapsed2.count() / num_trial;

  return results;
}

int Simulator::PerformPartitioning(
    PartitionPacketClassifier& ppc, const std::vector<Rule>& ruleset,
    std::map<std::string, std::string>& summary) {
  std::chrono::time_point<std::chrono::system_clock> start, end;
  std::chrono::duration<double> elapsed_seconds;
  start = std::chrono::system_clock::now();
  int b = ppc.ComputeNumberOfBuckets(ruleset);
  end = std::chrono::system_clock::now();
  elapsed_seconds = end - start;
  // printf("\tConstruction time: %f \n", elapsed_seconds.count());

  summary["ConstructionTime(s)"] = std::to_string(elapsed_seconds.count());
  summary["NumberOfPartitions"] = std::to_string(b);
  return b;
}
