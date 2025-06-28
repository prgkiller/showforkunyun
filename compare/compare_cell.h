#pragma once

#include <unordered_set>
#include <any>
#include "../parse/spice.h"
#include "../parse/layout.h"

typedef std::string GRAPH_NODE_NAME;

class CompareCell {
    private:
        friend class CompareNetlist;
    private:
        struct DeviceElement;
        struct NetElement;
        struct GraphNode {
            GRAPH_NODE_NAME name;
            HASH_VALUE oldColor;
            HASH_VALUE newColor;
            NETLIST_ID netlistId;
            uint32_t degree;
        };
        template <typename T>
        struct Bucket: public GraphNode {
            std::vector<T> graphNodes;
            template <typename U>
            bool operator> (const Bucket<U>& another) const {
                if (degree == 1 && another.degree == 1) {
                    return graphNodes.size() > another.graphNodes.size();
                } else if (degree != 1 && another.degree != 1) {
                    return graphNodes.size() > another.graphNodes.size();
                } else {
                    return degree == 1;
                }
            }
        };
        typedef Bucket<std::shared_ptr<DeviceElement> > DeviceBucket;
        typedef Bucket<std::shared_ptr<NetElement> > NetBucket;
        struct DeviceElement: public GraphNode {
            std::shared_ptr<Device> device;
            std::vector<std::pair<std::shared_ptr<NetElement>, PIN_MAGIC> > _connectNetElements;
            DeviceElement(const std::shared_ptr<Device>& device_, const NETLIST_ID& id_): device(device_) {
                name = device_->GetName();
                oldColor = 0;
                newColor = 0;
                netlistId = id_;
                degree = device_->GetConnectNets().size();
            }
        };
        struct NetElement: public GraphNode {
            std::shared_ptr<Net> net;
            std::vector<std::pair<std::weak_ptr<DeviceElement>, PIN_MAGIC> > _connectDeviceElements;
            NetElement(const std::shared_ptr<Net>& net_, const NETLIST_ID& id_): net(net_) {
                name = net_->GetName();
                oldColor = 0;
                newColor = 0;
                netlistId = id_;
                degree = net_->GetConnectDevices().size();
            }
        };
        struct DeviceElementHash {
            size_t operator() (const std::shared_ptr<DeviceElement>& deviceElement) const {
                return deviceElement->oldColor ^ deviceElement->newColor;
            }
        };
        struct DeviceElementEqual {
            bool operator() (const std::shared_ptr<DeviceElement>& a, const std::shared_ptr<DeviceElement>& b) const {
                return a->oldColor == b->oldColor && a->newColor == b->newColor;
            }
        };
        struct NetElementHash {
            size_t operator() (const std::shared_ptr<NetElement>& netElement) const {
                return netElement->oldColor ^ netElement->newColor;
            }
        };
        struct NetElementEqual {
            bool operator() (const std::shared_ptr<NetElement>& a, const std::shared_ptr<NetElement>& b) const {
                return a->newColor == b->newColor && a->oldColor == b->oldColor;
            }
        };
    private:
        std::shared_ptr<Cell> _cell1{nullptr}, _cell2{nullptr};
        std::vector<std::shared_ptr<DeviceElement> > _deviceElements;
        std::unordered_map<std::shared_ptr<Net>, std::shared_ptr<NetElement> > _nets;
        uint8_t _lastBucketsId{1}; // which buckets is the final iterate step
        std::unordered_map<std::shared_ptr<DeviceElement>, DeviceBucket, DeviceElementHash, DeviceElementEqual> _deviceBuckets[2];
        std::unordered_map<std::shared_ptr<NetElement>, NetBucket, NetElementHash, NetElementEqual> _netBuckets[2];

        std::vector<DeviceBucket*> _automorphismDeviceBuckets;
        std::vector<NetBucket*> _automorphismNetBuckets;
    private:
        void LoadData();
        bool AssignInitialBuckets();

        AUTOMORPHISM_GROUPS WeisfeilerLehman(); // WL algorithm
        ITERATE_STATUS Iterate(); // one iterate step

        void AssignBuckets();
        void AssignDeviceBuckets();
        void AssignNetBuckets();

        // color
        void IterateColor();
        void AssignNewDeviceColorToOld();
        void AssignNewNetColorToOld();
        void UpdateDevicesColor();
        void UpdateNetsColor();
        HASH_VALUE GetDeviceNewColor(const std::shared_ptr<DeviceElement>& deviceElement);
        HASH_VALUE GetNetNewColor(const std::shared_ptr<NetElement>& netElement);

        // check
        AUTOMORPHISM_GROUPS BucketsCheck();
        AUTOMORPHISM_GROUPS DeviceBucketsCheck(std::unordered_map<std::shared_ptr<DeviceElement>, DeviceBucket, DeviceElementHash, DeviceElementEqual>& buckets);
        AUTOMORPHISM_GROUPS NetBucketsCheck(std::unordered_map<std::shared_ptr<NetElement>, NetBucket, NetElementHash, NetElementEqual>& buckets);
        AUTOMORPHISM_GROUPS DeviceBucketCheck(DeviceBucket& bucket);
        AUTOMORPHISM_GROUPS NetBucketCheck(NetBucket& bucket);

        // automorphism
        COMPARE_CELL_RESULT ResolveAutomorphism();
        void ResolveAutomorphismByProperty();
        void ResolveAutomorphismByPin();
        void ResolveAutomorphismForce();
        void ResetDeviceBucketColor(DeviceBucket& bucket);
        void ResetNetBucketColor(NetBucket& bucket);
    public:
        CompareCell(const std::shared_ptr<Cell>& cell1, const std::shared_ptr<Cell>& cell2);
        COMPARE_CELL_RESULT Compare();
};