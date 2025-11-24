#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace pcap {

#pragma pack(push, 1)

struct PCAPFileHeader {
    uint32_t magic_number;
    uint16_t version_major;
    uint16_t version_minor;
    int32_t thiszone;
    uint32_t sigfigs;
    uint32_t snaplen;
    uint32_t network;
};

struct PCAPPacketHeader {
    uint32_t ts_sec;
    uint32_t ts_usec;
    uint32_t incl_len;
    uint32_t orig_len;
};

struct EthernetHeader {
    uint8_t dest_mac[6];
    uint8_t src_mac[6];
    uint16_t ethertype;
};

struct IPv4Header {
    uint8_t version_ihl;
    uint8_t dscp_ecn;
    uint16_t total_length;
    uint16_t identification;
    uint16_t flags_fragment;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t src_ip;
    uint32_t dst_ip;
};

struct UDPHeader {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t checksum;
};

#pragma pack(pop)

class PCAPReader {
    const uint8_t* data_;
    size_t size_;
    size_t offset_;
    PCAPFileHeader file_header_;
    bool valid_;
    
public:
    PCAPReader(const uint8_t* data, size_t size);
    
    bool is_valid() const { return valid_; }
    
    struct Packet {
        uint64_t timestamp_ns;
        const uint8_t* payload;
        size_t payload_size;
        uint16_t src_port;
        uint16_t dst_port;
    };
    
    bool next_packet(Packet& packet);
    void reset();
    size_t packets_read() const;
    
private:
    bool parse_headers(const uint8_t* packet_data, size_t packet_len, Packet& packet);
};

}
