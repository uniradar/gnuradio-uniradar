/* -*- c++ -*- */
/*
 * Copyright 2008,2009,2011,2013 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "tpb_thread_body.h"
#include <gnuradio/prefs.h>
#include <pmt/pmt.h>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <iostream>

namespace gr {

tpb_thread_body::tpb_thread_body(block_sptr block,
                                 gr::thread::barrier_sptr start_sync,
                                 int max_noutput_items)
    : d_exec(block, max_noutput_items)
{
    // std::cerr << "tpb_thread_body: " << block << std::endl;


#if defined(_MSC_VER) || defined(__MINGW32__)
#include <windows.h>
    thread::set_thread_name(
        GetCurrentThread(),
        boost::str(boost::format("%s%d") % block->name() % block->unique_id()));
#else
    thread::set_thread_name(
        pthread_self(),
        boost::str(boost::format("%s%d") % block->name() % block->unique_id()));
#endif

    block_detail* d = block->detail().get();
    block_executor::state s;
    pmt::pmt_t msg;

    d->threaded = true;
    d->thread = gr::thread::get_current_thread_id();

    prefs* p = prefs::singleton();
    size_t max_nmsgs = static_cast<size_t>(p->get_long("DEFAULT", "max_messages", 100));

// Setup the logger for the scheduler
#undef LOG
    std::string config_file = p->get_string("LOG", "log_config", "");
    std::string log_level = p->get_string("LOG", "log_level", "off");
    std::string log_file = p->get_string("LOG", "log_file", "");
    GR_LOG_GETLOGGER(LOG, "gr_log.tpb_thread_body");
    GR_LOG_SET_LEVEL(LOG, log_level);
    GR_CONFIG_LOGGER(config_file);
    if (log_file.size() > 0) {
        if (log_file == "stdout") {
            GR_LOG_SET_CONSOLE_APPENDER(LOG, "stdout", "gr::log :%p: %c{1} - %m%n");
        } else if (log_file == "stderr") {
            GR_LOG_SET_CONSOLE_APPENDER(LOG, "stderr", "gr::log :%p: %c{1} - %m%n");
        } else {
            GR_LOG_SET_FILE_APPENDER(LOG, log_file, true, "%r :%p: %c{1} - %m%n");
        }
    }

    // Set thread affinity if it was set before fg was started.
    if (block->processor_affinity().size() > 0) {
        gr::thread::thread_bind_to_processor(d->thread, block->processor_affinity());
    }

    // Set thread priority if it was set before fg was started
    if (block->thread_priority() > 0) {
        gr::thread::set_thread_priority(d->thread, block->thread_priority());
    }

    // make sure our block isnt finished
    block->clear_finished();

    start_sync->wait();
    while (1) {
        boost::this_thread::interruption_point();

        d->d_tpb.clear_changed();

        // handle any queued up messages
        BOOST_FOREACH (basic_block::msg_queue_map_t::value_type& i, block->msg_queue) {
            // Check if we have a message handler attached before getting
            // any messages. This is mostly a protection for the unknown
            // startup sequence of the threads.
            if (block->has_msg_handler(i.first)) {
                while ((msg = block->delete_head_nowait(i.first))) {
                    block->dispatch_msg(i.first, msg);
                }
            } else {
                // If we don't have a handler but are building up messages,
                // prune the queue from the front to keep memory in check.
                if (block->nmsgs(i.first) > max_nmsgs) {
                    GR_LOG_WARN(
                        LOG, "asynchronous message buffer overflowing, dropping message");
                    msg = block->delete_head_nowait(i.first);
                }
            }
        }

        // run one iteration if we are a connected stream block
        if (d->noutputs() > 0 || d->ninputs() > 0) {
            s = d_exec.run_one_iteration();
        } else {
            s = block_executor::BLKD_IN;
            // a msg port only block wants to shutdown
            if (block->finished()) {
                s = block_executor::DONE;
            }
        }

        if (block->finished() && s == block_executor::READY_NO_OUTPUT) {
            s = block_executor::DONE;
            d->set_done(true);
        }

        if (!d->ninputs() && s == block_executor::READY_NO_OUTPUT) {
            s = block_executor::BLKD_IN;
        }

        switch (s) {
        case block_executor::READY: // Tell neighbors we made progress.
            d->d_tpb.notify_neighbors(d);
            break;

        case block_executor::READY_NO_OUTPUT: // Notify upstream only
            d->d_tpb.notify_upstream(d);
            break;

        case block_executor::DONE: // Game over.
            block->notify_msg_neighbors();
            d->d_tpb.notify_neighbors(d);
            return;

        case block_executor::BLKD_IN: // Wait for input.
        {
            gr::thread::scoped_lock guard(d->d_tpb.mutex);

            if (!d->d_tpb.input_changed) {
                boost::system_time const timeout =
                    boost::get_system_time() + boost::posix_time::milliseconds(250);
                d->d_tpb.input_cond.timed_wait(guard, timeout);
            }
        } break;

        case block_executor::BLKD_OUT: // Wait for output buffer space.
        {
            gr::thread::scoped_lock guard(d->d_tpb.mutex);
            while (!d->d_tpb.output_changed) {
                d->d_tpb.output_cond.wait(guard);
            }
        } break;

        default:
            throw std::runtime_error("possible memory corruption in scheduler");
        }
    }
}

tpb_thread_body::~tpb_thread_body() {}

} /* namespace gr */
