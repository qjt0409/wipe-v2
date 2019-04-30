#include "dump.h"
#include <fstream>
#include <sstream>

std::string Dumper::TargetLoadsToStr(const Sim::Tunables &t, int cluster_idx) const {
    using namespace std;
    ostringstream buf;

    const auto &g = t.interactive[cluster_idx];

    auto get_freq      = [=](int idx) { return soc_.clusters_[cluster_idx].model_.opp_model[idx].freq; };
    int  n_opp         = soc_.clusters_[cluster_idx].model_.opp_model.size();
    int  n_targetloads = min(TARGET_LOAD_MAX_LEN, n_opp);

    int min_freq = soc_.clusters_[cluster_idx].model_.min_freq;
    int prev_tg  = -1;
    for (int i = 0; i < n_targetloads; ++i) {
        if (prev_tg == g.target_loads[i]) {
            continue;
        }
        if (get_freq(i) == min_freq) {
            buf << (int)g.target_loads[i];
            prev_tg = g.target_loads[i];
        } else if (get_freq(i) > min_freq) {
            buf << ' ' << Mhz2kHz(get_freq(i)) << ":" << (int)g.target_loads[i];
            prev_tg = g.target_loads[i];
        } else {
            continue;
        }
    }

    return buf.str();
}

std::string Dumper::HispeedDelayToStr(const Sim::Tunables &t, int cluster_idx) const {
    using namespace std;
    ostringstream buf;

    const auto &g = t.interactive[cluster_idx];

    auto get_freq       = [=](int idx) { return soc_.clusters_[cluster_idx].model_.opp_model[idx].freq; };
    auto multiple_to_us = [=](int multiple) { return Ms2Us(Quantum2Ms(multiple * t.sched.timer_rate) - 2); };
    int  n_opp          = soc_.clusters_[cluster_idx].model_.opp_model.size();
    int  n_above        = min(ABOVE_DELAY_MAX_LEN, n_opp) - 1;  // 最高频的above_delay并没有用

    int prev_above = -1;
    for (int i = 0; i < n_above; ++i) {
        if (prev_above == g.above_hispeed_delay[i]) {
            continue;
        }
        if (get_freq(i) == g.hispeed_freq) {
            buf << multiple_to_us(g.above_hispeed_delay[i]);
            prev_above = g.above_hispeed_delay[i];
        } else if (get_freq(i) > g.hispeed_freq) {
            buf << ' ' << Mhz2kHz(get_freq(i)) << ":" << multiple_to_us(g.above_hispeed_delay[i]);
            prev_above = g.above_hispeed_delay[i];
        } else {
            continue;
        }
    }
    buf << endl;

    return buf.str();
}

std::string Dumper::SimTunableToStr(const Sim::Tunables &t) const {
    using namespace std;
    ostringstream buf;

    int cluster_num = soc_.clusters_.size();

    for (int idx_cluster = 0; idx_cluster < cluster_num; ++idx_cluster) {
        const auto &g = t.interactive[idx_cluster];

        auto multiple_to_us = [=](int multiple) { return Ms2Us(Quantum2Ms(multiple * t.sched.timer_rate) - 2); };

        buf << "[interactive] cluster " << idx_cluster << endl << endl;
        buf << "hispeed_freq: " << Mhz2kHz(g.hispeed_freq) << endl;
        buf << "go_hispeed_load: " << g.go_hispeed_load << endl;
        buf << "min_sample_time: " << multiple_to_us(g.min_sample_time) << endl;
        buf << "max_freq_hysteresis: " << multiple_to_us(g.max_freq_hysteresis) << endl;

        buf << "above_hispeed_delay: ";
        buf << HispeedDelayToStr(t, idx_cluster);

        buf << "target_loads: ";
        buf << TargetLoadsToStr(t, idx_cluster);
        buf << endl << endl;
    }

    buf << "[hmp sched]" << endl << endl;
    buf << "sched_downmigrate: " << t.sched.sched_downmigrate << endl;
    buf << "sched_upmigrate: " << t.sched.sched_upmigrate << endl;
    buf << "sched_freq_aggregate_threshold_pct: " << t.sched.sched_freq_aggregate_threshold_pct << endl;
    buf << "sched_ravg_hist_size: " << t.sched.sched_ravg_hist_size << endl;
    buf << "sched_window_stats_policy: " << t.sched.sched_window_stats_policy << endl;
    buf << "timer_rate: " << Ms2Us(Quantum2Ms(t.sched.timer_rate)) << endl;
    buf << endl;

    buf << "[input boost]" << endl << endl;
    for (int idx_cluster = 0; idx_cluster < cluster_num; ++idx_cluster) {
        buf << "cluster " << idx_cluster << ": " << t.input.boost_freq[idx_cluster] << endl;
    }
    buf << "ms: " << Quantum2Ms(t.input.duration_quantum) << endl;
    buf << endl;

    return buf.str();
}

void Dumper::DumpToTXT(const std::vector<OpengaAdapter::Result> &results) const {
    using namespace std;
    string   filename = soc_.name_ + ".txt";
    ofstream ofs(output_path_ + filename);

    int idx_ind = 0;
    for (const auto &r : results) {
        ofs << "================" << endl << endl;
        ofs << ">>> " << idx_ind << " <<<" << endl;
        ofs << "performance: " << Double2Pct(r.score.performance) << endl;
        ofs << "battery_life: " << Double2Pct(r.score.battery_life) << endl;
        ofs << "idle_lasting: " << Double2Pct(r.score.idle_lasting) << endl;
        ofs << endl;
        ofs << SimTunableToStr(r.tunable);
        idx_ind++;
    }
    return;
}

void Dumper::DumpToCSV(const std::vector<OpengaAdapter::Result> &results) const {
    using namespace std;
    string   filename = soc_.name_ + ".csv";
    ofstream ofs(output_path_ + filename);

    int idx_ind = 0;
    for (const auto &r : results) {
        ofs << Double2Pct(r.score.performance) << ',';
        ofs << Double2Pct(r.score.battery_life) << ',';
        ofs << Double2Pct(r.score.idle_lasting) << ',';
        ofs << idx_ind;
        ofs << endl;
        idx_ind++;
    }
    return;
}
