/* -*- c++ -*- */
/* 
 * Copyright 2014 Analog Devices Inc.
 * Author: Paul Cercueil <paul.cercueil@analog.com>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cstdio>

#include <gnuradio/io_signature.h>
#include "fmcomms2_sink_impl.h"
#include "device_source_impl.h"

namespace gr {
  namespace iio {

    fmcomms2_sink::sptr
    fmcomms2_sink::make(const std::string &uri, unsigned long long frequency,
		    unsigned long samplerate, unsigned long interpolation,
		    unsigned long bandwidth,
		    bool ch1_en, bool ch2_en, bool ch3_en, bool ch4_en,
		    unsigned long buffer_size, bool cyclic,
		    const char *rf_port_select, double attenuation1,
		    double attenuation2, const char *filter,
		    const char *phyname, const char *txname)
    {
      return gnuradio::get_initial_sptr(
	    new fmcomms2_sink_impl(device_source_impl::get_context(uri), true,
		    frequency, samplerate, interpolation, bandwidth, ch1_en,
		    ch2_en, ch3_en, ch4_en, buffer_size, cyclic, rf_port_select,
		    attenuation1, attenuation2, filter, phyname, txname));
    }

    fmcomms2_sink::sptr
    fmcomms2_sink::make_from(struct iio_context *ctx,
		    unsigned long long frequency, unsigned long samplerate,
		    unsigned long interpolation, unsigned long bandwidth,
		    bool ch1_en, bool ch2_en, bool ch3_en, bool ch4_en,
		    unsigned long buffer_size, bool cyclic,
		    const char *rf_port_select, double attenuation1,
		    double attenuation2, const char *filter,
		    const char *phyname, const char *txname)
    {
      return gnuradio::get_initial_sptr(
	    new fmcomms2_sink_impl(ctx, false, frequency, samplerate,
		    interpolation, bandwidth, ch1_en, ch2_en, ch3_en, ch4_en,
		    buffer_size, cyclic, rf_port_select,
		    attenuation1, attenuation2, filter, phyname, txname));
    }

    std::vector<std::string> fmcomms2_sink_impl::get_channels_vector(
		    bool ch1_en, bool ch2_en, bool ch3_en, bool ch4_en)
    {
	    std::vector<std::string> channels;
	    if (ch1_en)
		    channels.push_back("voltage0");
	    if (ch2_en)
		    channels.push_back("voltage1");
	    if (ch3_en)
		    channels.push_back("voltage2");
	    if (ch4_en)
		    channels.push_back("voltage3");
	    return channels;
    }

    fmcomms2_sink_impl::fmcomms2_sink_impl(struct iio_context *ctx,
		    bool destroy_ctx,
		    unsigned long long frequency, unsigned long samplerate,
		    unsigned long interpolation, unsigned long bandwidth,
		    bool ch1_en, bool ch2_en, bool ch3_en, bool ch4_en,
		    unsigned long buffer_size, bool cyclic,
		    const char *rf_port_select, double attenuation1,
		    double attenuation2, const char *filter,
		    const char *phyname, const char *txname)
	    : gr::sync_block("fmcomms2_sink",
			    gr::io_signature::make(1, -1, sizeof(short)),
			    gr::io_signature::make(0, 0, 0)),
	      device_sink_impl(ctx, destroy_ctx, txname,
			    get_channels_vector(ch1_en, ch2_en, ch3_en, ch4_en),
			    phyname, std::vector<std::string>(),
			    buffer_size, interpolation, cyclic),
	      cyclic(cyclic)
    {
	    set_params(frequency, samplerate, bandwidth, rf_port_select,
			    attenuation1, attenuation2);

	    std::string f(filter);
	    if (!f.empty() && !device_source_impl::load_fir_filter(f, phy))
		    throw std::runtime_error("Unable to load filter file");
    }

    void fmcomms2_sink_impl::set_params(unsigned long long frequency,
		    unsigned long samplerate, unsigned long bandwidth,
		    const char *rf_port_select,
		    double attenuation1, double attenuation2)
    {
	    bool is_fmcomms4 = !iio_device_find_channel(phy, "voltage1", false);
	    std::vector<std::string> params;

	    params.push_back("out_altvoltage1_TX_LO_frequency=" +
			    boost::to_string(frequency));
	    params.push_back("out_voltage_sampling_frequency=" +
			    boost::to_string(samplerate));
	    params.push_back("out_voltage_rf_bandwidth=" +
			    boost::to_string(bandwidth));
	    params.push_back("out_voltage0_rf_port_select=" +
			    boost::to_string(rf_port_select));
	    params.push_back("out_voltage0_hardwaregain=" +
			    boost::to_string(-attenuation1));

	    if (!is_fmcomms4) {
		    params.push_back("out_voltage1_hardwaregain=" +
				    boost::to_string(-attenuation2));
	    }

	    device_source_impl::set_params(this->phy, params);
    }

    int fmcomms2_sink_impl::work(int noutput_items,
		    gr_vector_const_void_star &input_items,
		    gr_vector_void_star &output_items)
    {
	    int ret = device_sink_impl::work(noutput_items, input_items,
			    output_items);
	    if (ret < 0 || !cyclic)
		    return ret;
	    else
		    return WORK_DONE;
    }
  } /* namespace iio */
} /* namespace gr */
