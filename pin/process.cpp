#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
using namespace std;

inline bool classify(int64_t est) {
  return est > 50000000;
}

unsigned long long staleCt = 0;
unsigned long long plugCt = 0;
unsigned long long swatCt = 0;
unsigned long long plugCorrect = 0;
unsigned long long swatCorrect = 0;

map<unsigned long, uint64_t> swatDrag, plugDrag, realDrag, siteCt;

struct drag {
  uint64_t val;
  unsigned long site;

  drag(uint64_t d, unsigned long s) : val(d), site(s) {
  }
  bool operator<(const drag &rhs) const {
    return val < rhs.val;
  }
};

set<drag> plugSort, swatSort, realSort;

int main() {
  unsigned long ptr;
  uint64_t real, swat, plug, page;
  uint64_t realDragTotal = 0, plugDragTotal = 0, swatDragTotal = 0;
  unsigned long site, size;

  set<uint32_t> realSites, plugSites, swatSites, plugInter, swatInter;

  double pl_error = 0, sw_error = 0;
  unsigned int total = 0;

  cin >> std::hex;
  while (cin >> ptr >> real >> swat >> plug >> page >> site >> size) {
    // cout << std::hex << ptr << " " << real << " " << swat << " " << plug << endl;
    if (classify(real)) {
      staleCt++;
      realDrag[site] += real;  //*size;
      // realDragTotal += real;//*size

      realSites.insert(site);

      // printf("ob %p, stale %d\n",ptr,real);
    }
    if (classify(plug)) {
      plugCt++;
      plugDrag[site] += plug;  //*size;

      plugSites.insert(site);
    }
    if (classify(swat)) {
      swatCt++;
      swatDrag[site] += swat;  //*size;

      swatSites.insert(site);
    }

    if (classify(real) && classify(swat)) {
      swatCorrect++;
    }

    if (classify(real) && classify(plug)) {
      plugCorrect++;
      // plugDragTotal += real;//*size;
    }

    if (!classify(real) && classify(plug)) {
      cout << "BAD: " << std::hex << ptr << std::dec << endl;
    }

    swatDragTotal += real;  //*size;
    plugDragTotal += plug;
    realDragTotal += swat;

    siteCt[site]++;

    assert(real >= plug);
    assert(real <= swat);

    int64_t ple = (long long)real - (long long)plug;
    int64_t swe = (long long)swat - (long long)real;

    total++;

    pl_error += ple * ple;
    sw_error += swe * swe;
  }

  set_intersection(realSites.begin(), realSites.end(), plugSites.begin(), plugSites.end(),
                   inserter(plugInter, plugInter.begin()));
  set_intersection(realSites.begin(), realSites.end(), swatSites.begin(), swatSites.end(),
                   inserter(swatInter, swatInter.begin()));

  cout << "false positives (SWAT sites): " << swatSites.size() - swatInter.size() << "/" << realSites.size() << endl;

  cout << "Real: " << staleCt << endl;
  cout << "SWAT: " << swatCt << "(" << swatCorrect << ")" << endl;
  cout << "Plug: " << plugCt << "(" << plugCorrect << ")" << endl;

  cout << "Plug recall: " << ((double)plugCorrect / (double)staleCt)
       << " precision: " << ((double)plugCorrect / (double)plugCt) << endl;
  cout << "SWAT recall: " << ((double)swatCorrect / (double)staleCt)
       << " precision: " << ((double)swatCorrect / (double)swatCt) << endl;

  cout << endl;

  cout << "Plug MSE: " << pl_error / total << endl;
  cout << "Swat MSE: " << sw_error / total << endl;

  cout << "Plug ratio: " << (double)plugDragTotal / (double)realDragTotal << endl;
  cout << "SWAT ratio: " << (double)swatDragTotal / (double)realDragTotal << endl;

  {
    uint64_t npsum = 0, nssum = 0, dsum = 0;
    uint64_t psum = 0, ssum = 0;
    for (set<uint32_t>::iterator i = plugInter.begin(); i != plugInter.end(); i++) {
      // npsum += siteCt[*i];
      npsum += realDrag[*i];
    }
    for (set<uint32_t>::iterator i = swatInter.begin(); i != swatInter.end(); i++) {
      // nssum += siteCt[*i];
      nssum += realDrag[*i];
    }
    for (set<uint32_t>::iterator i = realSites.begin(); i != realSites.end(); i++) {
      // dsum += siteCt[*i];
      dsum += realDrag[*i];
    }
    for (set<uint32_t>::iterator i = plugSites.begin(); i != plugSites.end(); i++) {
      // psum += siteCt[*i];
      psum += realDrag[*i];
    }
    for (set<uint32_t>::iterator i = swatSites.begin(); i != swatSites.end(); i++) {
      // ssum += siteCt[*i];
      ssum += swatDrag[*i];
    }
    cout << "Plug sites recall (weighted): " << ((double)npsum / (double)dsum) << endl;
    cout << "SWAT sites recall (weighted): " << ((double)nssum / (double)dsum) << endl;
    cout << "Plug sites precision (weighted): " << ((double)npsum / (double)psum) << endl;
    cout << "SWAT sites precision (weighted): " << ((double)nssum / (double)ssum) << endl;

    cout << swatInter.size() << " " << swatSites.size() << endl;
  }

  // cout << "weighted plug recall: " << ((double)plugDragTotal/(double)realDragTotal) << endl;
  // cout << "weighted swat recall: " << ((double)swatDragTotal/(double)realDragTotal) << endl;
  /*
  cout << "wt plug prec: " << ((double)plugDragTotal/(double)realDragTotal) << endl;
  cout << "wt swat prec: " << ((double)swatDragTotal/(double)realDragTotal) << endl;
  */
  for (map<unsigned long, uint64_t>::iterator i = realDrag.begin(); i != realDrag.end(); i++) {
    realSort.insert(drag(i->second, i->first));
  }
  for (map<unsigned long, uint64_t>::iterator i = swatDrag.begin(); i != swatDrag.end(); i++) {
    swatSort.insert(drag(i->second, i->first));
  }
  for (map<unsigned long, uint64_t>::iterator i = plugDrag.begin(); i != plugDrag.end(); i++) {
    plugSort.insert(drag(i->second, i->first));
  }

  int ct = 0;

#define LIM 20

  for (set<drag>::reverse_iterator i = realSort.rbegin(); i != realSort.rend(); i++) {
    cout << std::hex << i->site << '\t' << std::dec << i->val << '\t' << swatDrag[i->site] << '\t' << plugDrag[i->site]
         << endl;
    if (++ct == LIM)
      break;
  }

  /*
    cout << " *****  REAL STALENESS ***** " << endl;
    for(set<drag>::reverse_iterator i = realSort.rbegin(); i != realSort.rend(); i++) {
      cout << std::hex << i->site << "\t" << std::dec << i->val << endl;
      if(++ct == LIM) break;
    }

    ct = 0;
    cout << " *****  PLUG DRAG  ***** " << endl;
    for(set<drag>::reverse_iterator i = plugSort.rbegin(); i != plugSort.rend(); i++) {
      cout << std::hex << i->site << "\t" << std::dec << i->val << endl;
      if(++ct == LIM) break;
    }

    ct = 0;
    cout << " *****  SWAT DRAG ***** " << endl;
    for(set<drag>::reverse_iterator i = swatSort.rbegin(); i != swatSort.rend(); i++) {
      cout << std::hex << i->site << "\t" << std::dec << i->val << endl;
      if(++ct == LIM) break;
    }
  */
}
