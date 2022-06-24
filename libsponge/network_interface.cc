#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();

    EthernetHeader header;
    header.src = _ethernet_address;
    header.type = EthernetHeader::TYPE_IPv4;

    EthernetFrame frame;
    frame.header() = header;
    frame.payload() = dgram.serialize();

    auto find_result = cache_.find(next_hop_ip);
    if (find_result != cache_.end() && find_result->second.valid_) {
        frame.header().dst = find_result->second.mac_address_;
        _frames_out.push(frame);
    } else {
        pending_data_[next_hop_ip].push_back(frame);
        if (find_result != cache_.end())
            // 说明存在cache中，且value为false，则由tick负责发送ARP请求。
            return;
        cache_[next_hop_ip] = {0, false};
        send_arp_request(next_hop_ip);
    }
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    auto &frame_header = frame.header();
    if (frame_header.dst != ETHERNET_BROADCAST && frame_header.dst != _ethernet_address)
        return {};
    if (frame_header.type == EthernetHeader::TYPE_ARP) {
        ARPMessage arp_msg;
        auto parser_result = arp_msg.parse(frame.payload());
        if (parser_result != ParseResult::NoError)
            return {};

        // cache the sender map
        // 对于reply类型的arp，这个敌方已经处理了
        cache_[arp_msg.sender_ip_address] = {0, true, arp_msg.sender_ethernet_address};
        // clear pending datagram
        for (auto &x : pending_data_[arp_msg.sender_ip_address]) {
            x.header().dst = arp_msg.sender_ethernet_address;
            _frames_out.push(x);
        }
        pending_data_[arp_msg.sender_ip_address].clear();

        if (arp_msg.opcode == ARPMessage::OPCODE_REQUEST && arp_msg.target_ip_address == _ip_address.ipv4_numeric()) {
            send_arp_reply(arp_msg.sender_ip_address, arp_msg.sender_ethernet_address);
        } 

        return {};
    } else if (frame_header.type == EthernetHeader::TYPE_IPv4) {
        IPv4Datagram data;
        auto parser_result = data.parse(frame.payload().concatenate());
        if (parser_result != ParseResult::NoError)
            return {};
        return data;
    } else {
        return {};
    }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) {
    for (auto it = cache_.begin(); it != cache_.end();) {
        it->second.time_since_last_send_ += ms_since_last_tick;
        if (it->second.valid_) {
            if (it->second.time_since_last_send_ > CacheEntry::EXPIRE_TIME) {
                it = cache_.erase(it);
                continue;
            }
        } else {
            if (it->second.time_since_last_send_ > CacheEntry::TIMEOUT_RESEND) {
                send_arp_request(it->first);
                it->second.time_since_last_send_ = 0;
            }
        }
        ++it;
    }
}

void NetworkInterface::send_arp_request(uint32_t des_ip) {
    EthernetFrame arp_frame;

    EthernetHeader arp_frame_header;
    arp_frame_header.type = EthernetHeader::TYPE_ARP;
    arp_frame_header.dst = ETHERNET_BROADCAST;
    arp_frame_header.src = _ethernet_address;
    arp_frame.header() = arp_frame_header;

    ARPMessage arp_msg;
    arp_msg.opcode = ARPMessage::OPCODE_REQUEST;
    arp_msg.sender_ethernet_address = _ethernet_address;
    arp_msg.sender_ip_address = _ip_address.ipv4_numeric();
    arp_msg.target_ip_address = des_ip;
    arp_frame.payload() = arp_msg.serialize();

    _frames_out.push(arp_frame);
}

void NetworkInterface::send_arp_reply(uint32_t dst_ip, EthernetAddress dst_mac_address) {
    // send arp reply
    EthernetFrame reply_arp_frame;

    EthernetHeader arp_frame_header;
    arp_frame_header.type = EthernetHeader::TYPE_ARP;
    arp_frame_header.dst = dst_mac_address;
    arp_frame_header.src = _ethernet_address;
    reply_arp_frame.header() = arp_frame_header;

    ARPMessage reply_arp_msg;
    reply_arp_msg.opcode = ARPMessage::OPCODE_REPLY;
    reply_arp_msg.sender_ethernet_address = _ethernet_address;
    reply_arp_msg.sender_ip_address = _ip_address.ipv4_numeric();
    reply_arp_msg.target_ip_address = dst_ip;
    reply_arp_msg.target_ethernet_address = dst_mac_address;
    reply_arp_frame.payload() = reply_arp_msg.serialize();

    _frames_out.push(reply_arp_frame);
}
