#include "pcap_reader.hpp"
#include <cstring>

namespace pcap {

static uint16_t ntohs_custom(uint16_t val) {
    return ((val & 0xFF) << 8) | ((val >> 8) & 0xFF);
}

static uint32_t ntohl_custom(uint32_t val) {
    return ((val & 0xFF) << 24) | 
           ((val & 0xFF00) << 8) | 
           ((val >> 8) & 0xFF00) | 
           ((val >> 24) & 0xFF);
}

PCAPReader::PCAPReader(const uint8_t* data, size_t size)
    : data_(data)
    , size_(size)
    , offset_(0)
    , valid_(false) {
    
    if (size < sizeof(PCAPFileHeader)) {
        return;
    }
    
    std::memcpy(&file_header_, data, sizeof(PCAPFileHeader));
    
    if (file_header_.magic_number != 0xa1b2c3d4 && 
        file_header_.magic_number != 0xd4c3b2a1) {
        return;
    }
    
    offset_ = sizeof(PCAPFileHeader);
    valid_ = true;
}

bool PCAPReader::next_packet(Packet& packet) {
    if (offset_ + sizeof(PCAPPacketHeader) > size_) {
        return false;
    }
    
    PCAPPacketHeader pkt_header;
    std::memcpy(&pkt_header, data_ + offset_, sizeof(PCAPPacketHeader));
    offset_ += sizeof(PCAPPacketHeader);
    
    if (offset_ + pkt_header.incl_len > size_) {
        return false;
    }
    
    packet.timestamp_ns = static_cast<uint64_t>(pkt_header.ts_sec) * 1000000000ULL +
                         static_cast<uint64_t>(pkt_header.ts_usec) * 1000ULL;
    
    if (!parse_headers(data_ + offset_, pkt_header.incl_len, packet)) {
        offset_ += pkt_header.incl_len;
        return false;
    }
    
    offset_ += pkt_header.incl_len;
    return true;
}

bool PCAPReader::parse_headers(const uint8_t* packet_data, size_t packet_len, Packet& packet) {
    size_t offset = 0;
    
    if (packet_len < sizeof(EthernetHeader)) {
        return false;
    }
    
    const EthernetHeader* eth = reinterpret_cast<const EthernetHeader*>(packet_data);
    offset += sizeof(EthernetHeader);
    
    uint16_t ethertype = ntohs_custom(eth->ethertype);
    if (ethertype != 0x0800) {
        return false;
    }
    
    if (offset + sizeof(IPv4Header) > packet_len) {
        return false;
    }
    
    const IPv4Header* ip = reinterpret_cast<const IPv4Header*>(packet_data + offset);
    uint8_t ihl = (ip->version_ihl & 0x0F) * 4;
    offset += ihl;
    
    if (ip->protocol != 17) {
        return false;
    }
    
    if (offset + sizeof(UDPHeader) > packet_len) {
        return false;
    }
    
    const UDPHeader* udp = reinterpret_cast<const UDPHeader*>(packet_data + offset);
    packet.src_port = ntohs_custom(udp->src_port);
    packet.dst_port = ntohs_custom(udp->dst_port);
    
    offset += sizeof(UDPHeader);
    
    if (offset >= packet_len) {
        return false;
    }
    
    packet.payload = packet_data + offset;
    packet.payload_size = packet_len - offset;
    
    return true;
}

void PCAPReader::reset() {
    offset_ = sizeof(PCAPFileHeader);
}

size_t PCAPReader::packets_read() const {
    return 0;
}

}
