/* -*- c++ -*- */
/*
 * Copyright 2013,2014,2019 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef INCLUDED_ZEROMQ_REQ_MSG_SOURCE_IMPL_H
#define INCLUDED_ZEROMQ_REQ_MSG_SOURCE_IMPL_H

#include "zmq_common_impl.h"
#include <gnuradio/zeromq/req_msg_source.h>
#include</usr/lib/python2.7/dist-packages/zmq/utils/zmq_constants.h>
namespace gr {
namespace zeromq {

class req_msg_source_impl : public req_msg_source
{
private:
    int d_timeout;
    zmq::context_t* d_context;
    zmq::socket_t* d_socket;
    boost::thread* d_thread;
    const pmt::pmt_t d_port;

    void readloop();

public:
    bool d_finished;

    req_msg_source_impl(char* address, int timeout);
    ~req_msg_source_impl();

    bool start();
    bool stop();

    std::string last_endpoint() override
    {
        char addr[256];
        size_t addr_len = sizeof(addr);
        d_socket->getsockopt(ZMQ_LAST_ENDPOINT, addr, &addr_len);
        return std::string(addr, addr_len - 1);
    }
};

} // namespace zeromq
} // namespace gr

#endif /* INCLUDED_ZEROMQ_REQ_MSG_SOURCE_IMPL_H */
