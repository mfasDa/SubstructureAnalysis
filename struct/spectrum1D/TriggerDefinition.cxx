#ifndef TRIGGERDEFINITION_CXX
#define TRIGGERDEFINITION_CXX

#include "../../meta/stl.C"
#include "../../meta/root.C"

enum class TriggerCluster {
    kANY,
    kCENT,
    kCENTNOTRD
};

class TriggerClusterBinned {
    public:
        TriggerClusterBinned() = default;
        ~TriggerClusterBinned() = default;

        int getClusterBin(TriggerCluster clust) const {
            switch (clust) {
                case TriggerCluster::kANY: return 1;
                case TriggerCluster::kCENT: return 2;
                case TriggerCluster::kCENTNOTRD: return 3;
            };
        }
};


#endif
