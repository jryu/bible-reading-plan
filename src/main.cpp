#include <algorithm>
#include <fstream>
#include <gflags/gflags.h>
#include <iomanip>
#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <set>
#include <string>
#include <sstream>
#include <signal.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <execinfo.h>
#include <math.h>
#include <unistd.h>
#include <vector>

DEFINE_int32(days_to_read, 365, "Number of days to read");
DEFINE_string(reading_units_file, "reading-units/reading-units.csv", "");
DEFINE_string(fixed_plan_file, "", "A plan file to read together.");

auto console = spdlog::stdout_color_mt("console");

template <typename Out>
void split(const std::string &s, char delim, Out result) {
  std::istringstream iss(s);
  std::string item;
  while (std::getline(iss, item, delim)) {
    *result++ = item;
  }
}

std::vector<std::string> split(const std::string &s, char delim) {
  std::vector<std::string> elems;
  split(s, delim, std::back_inserter(elems));
  return elems;
}

void handler(int sig) {
  void *array[10];
  size_t size;

  // get void*'s for all entries on the stack
  size = backtrace(array, 10);

  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}

std::map<std::string, int> book_to_chapter_count = {
  {"Genesis", 50},
  {"Exodus", 40},
  {"Leviticus", 27},
  {"Numbers", 36},
  {"Deuteronomy", 34},
  {"Joshua", 24},
  {"Judges", 21},
  {"Ruth", 4},
  {"1 Samuel", 31},
  {"2 Samuel", 24},
  {"1 Kings", 22},
  {"2 Kings", 25},
  {"1 Chronicles", 29},
  {"2 Chronicles", 36},
  {"Ezra", 10},
  {"Nehemiah", 13},
  {"Esther", 10},
  {"Job", 42},
  {"Psalms", 150},
  {"Proverbs", 31},
  {"Ecclesiastes", 12},
  {"Song of Solomon", 8},
  {"Isaiah", 66},
  {"Jeremiah", 52},
  {"Lamentations", 5},
  {"Ezekiel", 48},
  {"Daniel", 12},
  {"Hosea", 14},
  {"Joel", 3},
  {"Amos", 9},
  {"Obadiah", 1},
  {"Jonah", 4},
  {"Micah", 7},
  {"Nahum", 3},
  {"Habakkuk", 3},
  {"Zephaniah", 3},
  {"Haggai", 2},
  {"Zechariah", 14},
  {"Malachi", 4},
  {"Matthew", 28},
  {"Mark", 16},
  {"Luke", 24},
  {"John", 21},
  {"Acts", 28},
  {"Romans", 16},
  {"1 Corinthians", 16},
  {"2 Corinthians", 13},
  {"Galatians", 6},
  {"Ephesians", 6},
  {"Philippians", 4},
  {"Colossians", 4},
  {"1 Thessalonians", 5},
  {"2 Thessalonians", 3},
  {"1 Timothy", 6},
  {"2 Timothy", 4},
  {"Titus", 3},
  {"Philemon", 1},
  {"Hebrews", 13},
  {"James", 5},
  {"1 Peter", 5},
  {"2 Peter", 3},
  {"1 John", 5},
  {"2 John", 1},
  {"3 John", 1},
  {"Jude", 1},
  {"Revelation", 22}
};

struct ReadingUnit {
  ReadingUnit() :
    verse_from(-1), verse_to(-1),
    end_of_chapter(true) {}

  std::string book;
  int chapter;
  int verse_from;
  int verse_to;
  int num_verses;
  bool end_of_chapter;
};

class DailyReading {
  public:
    DailyReading() : num_verses_(0) {}

    void PushFront(ReadingUnit reading_unit) {
      num_verses_ += reading_unit.num_verses;
      reading_units_.push_front(std::move(reading_unit));
    }

    void PushBack(ReadingUnit reading_unit) {
      num_verses_ += reading_unit.num_verses;
      reading_units_.push_back(std::move(reading_unit));
    }

    ReadingUnit PopFront() {
      ReadingUnit front = reading_units_.front();
      num_verses_ -= front.num_verses;
      reading_units_.pop_front();
      return front;
    }

    ReadingUnit PopBack() {
      ReadingUnit back = reading_units_.back();
      num_verses_ -= back.num_verses;
      reading_units_.pop_back();
      return back;
    }

    void set_num_verses(int value) { num_verses_ = value; }
    int num_verses() { return num_verses_; }

    bool CanSplit() { return reading_units_.size() > 1; }

    DailyReading Split() {
      DailyReading another_one;
      another_one.PushBack(PopFront());

      while (another_one.num_verses() < num_verses()) {
        int front_verses = reading_units_.front().num_verses;
        int diff_current = abs(num_verses() - another_one.num_verses());
        int diff_new = abs(another_one.num_verses() + front_verses -
            (num_verses() - front_verses));
        if (diff_new > diff_current) {
          break;
        }
        another_one.PushBack(PopFront());
      }
      return another_one;
    }

    bool EndsAtEndOfChapter() {
      return reading_units_.back().end_of_chapter;
    }

    void Print() {
      const auto& front = reading_units_.front();
      const auto& back = reading_units_.back();
      std::cout << front.book << ',' << front.chapter << ',';
      bool should_print_verse_of_front = front.verse_from > 1 || (
          front.verse_from == 1 &&
          front.book == back.book &&
          front.chapter == back.chapter &&
          !back.end_of_chapter);
      if (should_print_verse_of_front) {
        std::cout << front.verse_from;
      }
      if (reading_units_.size() == 1 || (
            front.book == back.book &&
            front.chapter == back.chapter)) {
        if (should_print_verse_of_front) {
          std::cout << ',' << front.book << ',' << front.chapter << ',' << back.verse_to;
        } else {
          std::cout << ",,,";
        }
      } else {
        std::cout << ',' << back.book << ',' << back.chapter << ',';
        if (!back.end_of_chapter) {
          std::cout << back.verse_to;
        }
      }
    }

  private:
    std::list<ReadingUnit> reading_units_;
    int num_verses_;
};

class ReadingPlan {
  public:
    ReadingPlan() : fixed_plan_total_num_verses_(0) {
    }

    void PushBack(DailyReading daily_reading) {
      daily_readings_.push_back(std::move(daily_reading));
    }

    void PushBackFixedPlan(DailyReading daily_reading) {
      fixed_plan_total_num_verses_ += daily_reading.num_verses();
      fixed_daily_readings_.push_back(std::move(daily_reading));
    }

    int GetFixedPlanDiffVersesOnDay(int day_index) {
      if (day_index < fixed_daily_readings_.size()) {
        return fixed_daily_readings_[day_index].num_verses() -
          fixed_plan_total_num_verses_ /
          fixed_daily_readings_.size();
      }
      return 0;
    }

    bool FindAndSplit() {
      int max_weight = 0;
      std::list<DailyReading>::iterator max_it = daily_readings_.end();
      std::list<DailyReading>::iterator it = daily_readings_.begin();
      std::vector<DailyReading>::iterator it_fixed = fixed_daily_readings_.begin();
      while (it != daily_readings_.end()) {
        int total_verses = it->num_verses();
        if (!fixed_daily_readings_.empty()) {
          total_verses += it_fixed->num_verses();
        }
        if (it->CanSplit() && total_verses > max_weight) {
          max_weight = total_verses;
          max_it = it;
        }
        ++it;
        if (!fixed_daily_readings_.empty()) {
          ++it_fixed;
        }
      }
      if (max_it == daily_readings_.end()) return false;

      auto new_daily_reading = max_it->Split();
      daily_readings_.insert(max_it, std::move(new_daily_reading));
      return true;
    }

    size_t size() { return daily_readings_.size(); }

    int MinWeight() {
      std::list<DailyReading>::iterator it = daily_readings_.begin();
      std::vector<DailyReading>::iterator it_fixed = fixed_daily_readings_.begin();
      int min_weight = std::numeric_limits<int>::max();
      while (it != daily_readings_.end()) {
        int total_verses = it->num_verses();
        if (!fixed_daily_readings_.empty()) {
          total_verses += it_fixed->num_verses();
        }
        if (min_weight > total_verses) {
          min_weight = total_verses;
        }
        ++it;
        if (!fixed_daily_readings_.empty()) {
          ++it_fixed;
        }
      }
      return min_weight;
    }

    int MaxWeight() {
      std::list<DailyReading>::iterator it = daily_readings_.begin();
      std::vector<DailyReading>::iterator it_fixed = fixed_daily_readings_.begin();
      int max_weight = 0;
      while (it != daily_readings_.end()) {
        int total_verses = it->num_verses();
        if (!fixed_daily_readings_.empty()) {
          total_verses += it_fixed->num_verses();
        }
        if (max_weight < total_verses) {
          max_weight = total_verses;
        }
        ++it;
        if (!fixed_daily_readings_.empty()) {
          ++it_fixed;
        }
      }
      return max_weight;
    }

    void Print() {
      std::list<DailyReading>::iterator it = daily_readings_.begin();
      std::vector<DailyReading>::iterator it_fixed = fixed_daily_readings_.begin();
      while (it != daily_readings_.end()) {
        auto total_verses = it->num_verses();
        if (!fixed_daily_readings_.empty()) {
          total_verses += it_fixed->num_verses();
        }
        std::cout << total_verses << ',';
        it->Print();
        std::cout << ',';
        if (fixed_daily_readings_.empty()) {
          if (!it->EndsAtEndOfChapter()) {
            std::cout << "false";
          }
        } else {
          it_fixed->Print();
        }
        std::cout << std::endl;
        ++it;
        if (!fixed_daily_readings_.empty()) {
          ++it_fixed;
        }
      }
    }

  private:
    std::list<DailyReading> daily_readings_;
    std::vector<DailyReading> fixed_daily_readings_;
    int fixed_plan_total_num_verses_;
};

class SubProblem {
  public:
    SubProblem() : weight_(-1) {}

    void SetWeight(int diff_verses, int book_switch_weight) {
      diff_verses_ = diff_verses;
      book_switch_weight_ = std::pow(book_switch_weight, 2);
      weight_ = std::pow(diff_verses, 2) + book_switch_weight_;
    }

    void set_weight(long weight) {
      weight_ = weight;
    }

    long weight() {
      return weight_;
    }

    long GetWeightWithMoreDiffVerses(int diff_verses) {
      return std::pow(diff_verses_ + diff_verses, 2) +
        book_switch_weight_;
    }

    int end_of_left;

  private:
    int diff_verses_;
    int book_switch_weight_;
    long weight_;
};

class SubProblems {
  public:
    SubProblems() {
      for (int i = 0; i < 365; ++i) {
        for (int j = 0; j < 1250; ++j) {
          for (int k = 0; k < 1250; ++k) {
            contains_[i][j][k] = false;
          }
        }
      }
    }
    bool Contains(int day, int from, int to) {
      return contains_[day][from][to];
    }

    SubProblem Get(int day, int from, int to) {
      return sub_problems_[day][from][to];
    }

    void Set(int day, int from, int to, SubProblem sub_problem) {
      if (sub_problems_.find(day) == sub_problems_.end()) {
        std::map<int, std::map<int, SubProblem>> tmp;
        sub_problems_[day] = tmp;
      }
      if (sub_problems_[day].find(from) == sub_problems_[day].end()) {
        std::map<int, SubProblem> tmp;
        sub_problems_[day][from] = tmp;
      }
      sub_problems_[day][from][to] = sub_problem;
      contains_[day][from][to] = true;
    }

  private:
    std::map<int, std::map<int, std::map<int, SubProblem>>> sub_problems_;
    bool contains_[365][1250][1250];
};

SubProblems sub_problems;

bool TraceSplits(int day, int from, int to, std::set<int>* splits) {
  console->debug("tracing splits: {} [{}, {}]",
      day, from, to);
  if (!sub_problems.Contains(day, from, to)) {
    console->error("There is no solution.");
    return false;
  }
  int end_of_left = sub_problems.Get(day, from, to).end_of_left;
  splits->insert(end_of_left);

  if (day == 0) return true;

  return TraceSplits(day - 1, from, end_of_left, splits) &&
    TraceSplits(0, end_of_left + 1, to, splits);
}

bool endsWith(std::string string, std::string token) {
  if (string.length() < token.length()) {
    return false;
  }
  return string.compare(string.length() - token.length(), token.length(),
      token) == 0;
}

std::vector<ReadingUnit> GetReadingUnits(std::string file_name)
{
  std::ifstream infile(file_name);
  if (!infile) {
    console->error("{} error: {}", file_name, strerror(errno));
    exit(errno);
  }
  std::string line;
  std::vector<ReadingUnit> reading_units;
  while (std::getline(infile, line)) {
    auto tokens = split(line, ',');
    ReadingUnit reading_unit;
    reading_unit.book = tokens[0];
    reading_unit.chapter = std::stoi(tokens[1]);
    if (!tokens[2].empty()) {
      reading_unit.verse_from = std::stoi(tokens[2]);
      reading_unit.verse_to = std::stoi(tokens[3]);
    }
    reading_unit.num_verses = std::stoi(tokens[4]);
    if (tokens.size() > 5 && !tokens[5].empty()) {
      reading_unit.end_of_chapter = false;
    }

    reading_units.push_back(reading_unit);
  }
  return reading_units;
}

ReadingPlan GetReadingPlan(std::string file_name)
{
  if (file_name.empty()) return ReadingPlan();

  std::ifstream infile(file_name);
  if (!infile) {
    console->error("{} error: {}", file_name, strerror(errno));
    exit(errno);
  }
  std::string line;
  ReadingPlan reading_plan;
  while (std::getline(infile, line)) {
    DailyReading daily_reading;
    bool end_of_chapter = true;
    auto tokens = split(line, ',');
    if (tokens.size() >= 8 && !tokens[7].empty()) {
      end_of_chapter = false;
    }
    {
      ReadingUnit reading_unit;
      reading_unit.book = tokens[1];
      reading_unit.chapter = std::stoi(tokens[2]);
      if (!tokens[3].empty()) {
        reading_unit.verse_from = std::stoi(tokens[3]);
        reading_unit.verse_to = std::stoi(tokens[6]);
      }
      if (!end_of_chapter) reading_unit.end_of_chapter = false;
      daily_reading.PushBack(reading_unit);
    }
    if (!tokens[4].empty() &&
        (tokens.size() < 7 || tokens[6].empty())) {
      ReadingUnit reading_unit;
      reading_unit.book = tokens[4];
      reading_unit.chapter = std::stoi(tokens[5]);
      if (!end_of_chapter) reading_unit.end_of_chapter = false;
      daily_reading.PushBack(reading_unit);
    }
    daily_reading.set_num_verses(std::stoi(tokens[0]));
    reading_plan.PushBackFixedPlan(daily_reading);
  }
  return reading_plan;
}

int main(int argc, char **argv)
{
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  signal(SIGSEGV, handler);
  // console->set_level(spdlog::level::debug);

  std::vector<ReadingUnit> reading_units =
    GetReadingUnits(FLAGS_reading_units_file);
  DailyReading daily_reading;
  for (const auto& reading_unit : reading_units) {
    daily_reading.PushBack(reading_unit);
  }
  console->debug(daily_reading.num_verses());
  int days_total = FLAGS_days_to_read;
  long avg = daily_reading.num_verses() / days_total;

  ReadingPlan reading_plan = GetReadingPlan(FLAGS_fixed_plan_file);
  reading_plan.PushBack(std::move(daily_reading));

  while(reading_plan.size() < days_total) {
    reading_plan.FindAndSplit();
  }

  // find min & max
  int min_weight = reading_plan.MinWeight();
  int max_weight = reading_plan.MaxWeight();
  console->debug("{}, ({}), {}, ({})",
      min_weight, avg - min_weight,
      max_weight, max_weight - avg);
  long worst = std::max(avg - min_weight, max_weight - avg);

  for (int from = 0; from < reading_units.size(); ++from) {
    long total_verses = 0;
    long book_switch_weight = 0;
    std::string book = reading_units[from].book;
    for (int to = from; to < reading_units.size(); ++to) {
      total_verses += reading_units[to].num_verses;
      if (book != reading_units[to].book) {
        // Don't allow old testment & new testement combined in a single day
        if (book == "Malachi") {
          break;
        }

        auto next_book = reading_units[to].book;
        // Amos is between Joel and Obadiah, and it's too long to be
        // read in a single day. Let its first half to be grouped
        // together with Joel by not adding book_switch_weight.
        //
        // 2 John, 3 John and Jude are better to be grouped together.
        // Otherwise, 1 John and 2 John will be grouped togetehr
        if (next_book != "Amos" && next_book != "Jude") {
          book_switch_weight += book_to_chapter_count[book];
          book_switch_weight += book_to_chapter_count[next_book];
          book_switch_weight += 50;
        }
        book = next_book;
      } else if (
          (reading_units[from].book != reading_units[to].book ||
           reading_units[from].chapter != reading_units[to].chapter) &&
          ((from > 0 &&
            reading_units[from].book == reading_units[from - 1].book &&
            reading_units[from].chapter == reading_units[from - 1].chapter) ||
           (to + 1 < reading_units.size() &&
            reading_units[to].book == reading_units[to + 1].book &&
            reading_units[to].chapter == reading_units[to + 1].chapter))) {
        // Prevent multiple pieces of a chapter grouped with other chapters.
        book_switch_weight += 20;
      }
      long diff_verses = total_verses - avg;
      if (diff_verses <= worst) {
        SubProblem sub_problem;
        sub_problem.SetWeight(diff_verses, book_switch_weight);
        sub_problem.end_of_left = to;
        sub_problems.Set(0, from, to, sub_problem);
      }
    }
  }

  for (int day = 1; day < days_total; ++day) {
    int count_set = 0;
    int max_range = 0;
    long worst_weight = 0;
    int to_max = 0;
    for (int to = 0; to < reading_units.size(); ++to) {
      long min_weight = std::numeric_limits<long>::max();
      int min_i = -1;
      for (int i = 0; i < to; ++i) {
        if (!sub_problems.Contains(day - 1, 0, i) ||
            !sub_problems.Contains(0, i + 1, to)) {
          continue;
        }
        /* new weight = std::pow(total sum of verses + verses from fixed plan, 2); */
        /* Date of fixed plan comes from 'day' which is 0 based number
         * of days to read the plan */
        auto weight = sub_problems.Get(day - 1, 0, i).weight();

        weight +=
          sub_problems.Get(0, i + 1, to).GetWeightWithMoreDiffVerses(
              reading_plan.GetFixedPlanDiffVersesOnDay(day));
        if (min_weight > weight) {
          min_weight = weight;
          min_i = i;
        }
      }
      if (min_i != -1) {
        to_max = std::max(to_max, to);
        SubProblem sub_problem;
        sub_problem.set_weight(min_weight);
        sub_problem.end_of_left = min_i;
        sub_problems.Set(day, 0, to, sub_problem);

        max_range = std::max(max_range, to);
        worst_weight = std::max(worst_weight, min_weight);
        count_set++;
      }
    }
    if (sub_problems.Contains(day, 0, reading_units.size() - 1)) {
      console->debug(sub_problems.Get(day, 0, reading_units.size() - 1).weight());
    }
  }
  std::set<int> splits;
  if (TraceSplits(days_total - 1, 0, reading_units.size() - 1, &splits)) {
    ReadingPlan plan = GetReadingPlan(FLAGS_fixed_plan_file);
    int i = 0;
    for (auto it = splits.begin(); it != splits.end(); ++it) {
      DailyReading daily_reading;
      while (i <= *it) {
        daily_reading.PushBack(reading_units[i]);
        ++i;
      }
      plan.PushBack(daily_reading);
    }
    plan.Print();
    console->debug(plan.size());
  }

  return 0;
}
