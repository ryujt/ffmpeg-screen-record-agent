#pragma once

#include <ryulib/debug_tools.hpp>
#include <ryulib/MemoryBuffer.hpp>

extern "C" {
	#include <libswresample/swresample.h>
	#include <libavutil/opt.h>
}

#pragma comment(lib, "bcrypt")

class AudioResampling {
public:
	bool open(int in_channels, int in_samples, int in_sample_size, int out_channels, int out_samples, int out_sample_size, int frames)
	{
		close();

		memory_buffer_.clear();

		src_channels_ = in_channels;
		src_samples_ = in_samples;
		src_sample_size_ = in_sample_size;
		dst_channels_ = out_channels;
		dst_samples_ = out_samples;
		dst_sample_size_ = out_sample_size;

		dst_data_size = frames * dst_channels_ * dst_sample_size_;

		swr_ctx = swr_alloc();
		if (!swr_ctx) {
			DebugOutput::trace("Could not allocate resampler context");
			fin_values();
			return false;
		}

		switch (in_channels) {
			case 1: src_ch_layout = AV_CH_LAYOUT_MONO; break;
			case 2: src_ch_layout = AV_CH_LAYOUT_STEREO; break;
			default:
			{
				DebugOutput::trace("Error - in_channels");
				fin_values();
				return false;
			}
		}

		switch (out_channels) {
			case 1: dst_ch_layout = AV_CH_LAYOUT_MONO; break;
			case 2: dst_ch_layout = AV_CH_LAYOUT_STEREO; break;
			default:
			{
				DebugOutput::trace("Error - out_channels");
				fin_values();
				return false;
			}
		}

		switch (in_sample_size) {
			case 2: src_sample_fmt = AV_SAMPLE_FMT_S16; break;
			case 4: src_sample_fmt = AV_SAMPLE_FMT_FLT; break;
			default:
			{
				DebugOutput::trace("Error - in_sample_size");
				fin_values();
				return false;
			}
		}

		switch (out_sample_size) {
			case 2: dst_sample_fmt = AV_SAMPLE_FMT_S16; break;
			case 4: dst_sample_fmt = AV_SAMPLE_FMT_FLT; break;
			default:
			{
				DebugOutput::trace("Error - out_sample_size");
				fin_values();
				return false;
			}
		}

		av_opt_set_int(swr_ctx, "in_channel_count", src_channels_, 0);
		av_opt_set_int(swr_ctx, "in_sample_rate", src_samples_, 0);
		av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", src_sample_fmt, 0);

		av_opt_set_int(swr_ctx, "out_channel_count", dst_channels_, 0);
		av_opt_set_int(swr_ctx, "out_sample_rate", dst_samples_, 0);
		av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", dst_sample_fmt, 0);

		int ret = swr_init(swr_ctx);
		if (ret < 0) {
			DebugOutput::trace("Failed to initialize the resampling context");
			fin_values();
			return false;
		}

		max_dst_nb_samples = dst_nb_samples =
			av_rescale_rnd(src_nb_samples, dst_samples_, src_samples_, AV_ROUND_UP);

		ret = av_samples_alloc_array_and_samples(&dst_data, &dst_linesize, dst_channels_,
			dst_nb_samples, dst_sample_fmt, 0);
		if (ret < 0) {
			DebugOutput::trace("Could not allocate destination samples");
			return false;
		}

		dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, src_samples_) + src_nb_samples, dst_samples_, src_samples_, AV_ROUND_UP);
		if (dst_nb_samples > max_dst_nb_samples) {
			av_freep(&dst_data[0]);
			ret = av_samples_alloc(dst_data, &dst_linesize, dst_channels_,
				dst_nb_samples, dst_sample_fmt, 1);
			if (ret < 0) {
				return false;
			}
			max_dst_nb_samples = dst_nb_samples;
		}

		return true;
	}

	void close()
	{
		fin_values();
	}

	void execute(void* data, int size)
	{
		if (swr_ctx == nullptr) return;

		int ret = swr_convert(swr_ctx, dst_data, dst_nb_samples, (const uint8_t**) &data, src_nb_samples);
		if (ret < 0) {
			DebugOutput::trace("Error while converting");
			return;
		}

		int dst_bufsize = av_samples_get_buffer_size(&dst_linesize, dst_channels_, ret, dst_sample_fmt, 1);
		if (dst_bufsize < 0) {
			DebugOutput::trace("Could not get sample buffer size");
			return;
		}

//		DebugOutput::trace("in: %d,  out: %d, %d\n", src_nb_samples, ret, dst_bufsize);

		memory_buffer_.write(*dst_data, dst_bufsize);
		void* dst = memory_buffer_.read(dst_data_size);
		if (on_data_ != nullptr) on_data_(this, dst, dst_data_size);
		delete(dst);
	}

	int getSrcBufferSize() { return src_nb_samples * src_channels_ * src_sample_size_; }

	void setOnData(DataEvent event) { on_data_ = event; };

private:
	MemoryBuffer memory_buffer_;

	int src_channels_, src_samples_, src_sample_size_;
	int dst_channels_, dst_samples_, dst_sample_size_;
	int dst_data_size;

	struct SwrContext* swr_ctx = nullptr;
	enum AVSampleFormat src_sample_fmt = AV_SAMPLE_FMT_FLT, dst_sample_fmt = AV_SAMPLE_FMT_FLT;
	int64_t src_ch_layout = AV_CH_LAYOUT_STEREO, dst_ch_layout = AV_CH_LAYOUT_MONO;
	uint8_t** dst_data = nullptr;
	int src_linesize, dst_linesize;
	int src_nb_samples = 1024, dst_nb_samples, max_dst_nb_samples;

	DataEvent on_data_ = nullptr;

	void fin_values()
	{
		if (dst_data != nullptr) {
			av_freep(&dst_data[0]);
			av_freep(&dst_data);
			dst_data = nullptr;
		}

		if (swr_ctx != nullptr) {
			swr_free(&swr_ctx);
			swr_ctx = nullptr;
		}
	}
};
