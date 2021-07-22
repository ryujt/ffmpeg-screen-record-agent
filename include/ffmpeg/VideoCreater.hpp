#ifndef VIDEO_CREATER_HPP
#define VIDEO_CREATER_HPP

#include <ryulib/debug_tools.hpp>
#include <ryulib/MemoryBuffer.hpp>
#include <string>
#include <boost/scope_exit.hpp>

extern "C" {
#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

#pragma comment(lib, "ws2_32")
#pragma comment(lib, "secur32")
#pragma comment(lib, "bcrypt")
#pragma comment(lib, "avcodec")
#pragma comment(lib, "avdevice")
#pragma comment(lib, "avfilter")
#pragma comment(lib, "avformat")
#pragma comment(lib, "avresample")
#pragma comment(lib, "avutil")
#pragma comment(lib, "swresample")
#pragma comment(lib, "swscale")
#pragma comment(lib, "libx264")

using namespace std;

#define DEFAULT_FPS			24
#define DEFAULT_BITRATE_KB	1024
#define DEFAULT_CHANNEL		1
#define DEFAULT_SAMPLE_RATE	44100
#define DEFAULT_GOP_SIZE	16

typedef struct VideoCreaterOption {
	string filename;
	int width = 0;
	int height = 0;
	int fps	= DEFAULT_FPS;
	string speed = "fast";
	int bitrate = DEFAULT_BITRATE_KB * 1024;
	int gop_size = DEFAULT_GOP_SIZE;
	bool use_nvenc = true;

	int channel		= DEFAULT_CHANNEL;
	int sample_rate = DEFAULT_SAMPLE_RATE;

	uint64_t getCannelLayout()
	{
		if (channel == 1) return AV_CH_LAYOUT_MONO;
		else return AV_CH_LAYOUT_STEREO;
	}
};

typedef struct OutputStream {
	AVStream* st = nullptr;
	AVCodecContext* enc = nullptr;

	int64_t next_pts;
	int samples_count;

	int nb_samples;

	struct SwsContext* sws_ctx = nullptr;
	struct SwrContext* swr_ctx = nullptr;
} OutputStream;

class VideoCreater {
private:
	const static int PIXEL_SIZE = 4;
	const static AVSampleFormat AUDIO_FORMAT = AV_SAMPLE_FMT_FLT;
	const static int AUDIO_SAMPLE_SIZE = 4;
	const static int BITMAP_CELL_WIDTH = 8;
	const static int BITMAP_CELL_HEIGHT = 2;
	const static int BITMAP_PIXEL_SIZE = 4;

public:
	VideoCreater()
	{
	}

	~VideoCreater()
	{
		close();
	}

	/** VideoCreater 생성자
	@param option 생성할 비디오의 크기 등의 옵션들
	*/
	bool open(VideoCreaterOption option)
	{
		is_opened_ = false;

		option_ = option;

		avformat_alloc_output_context2(&oc, NULL, NULL, option_.filename.c_str());
		if (!oc) {
			DebugOutput::trace("createVideo - error avformat_alloc_output_context2");
			return false;
		}

		fmt = oc->oformat;
		if (fmt->video_codec == AV_CODEC_ID_NONE) {
			DebugOutput::trace("createVideo - fmt->video_codec == AV_CODEC_ID_NONE");
			return false;
		}

		using_hw_encoder_ = true;
		video_codec = avcodec_find_encoder_by_name("h264_nvenc");
		if ((option.use_nvenc == false) || (!video_codec) || (open_video() == false)) {
			using_hw_encoder_ = false;
			video_codec = avcodec_find_encoder(fmt->video_codec);
			if (!video_codec) {
				DebugOutput::trace("Could not find encoder for '%s'", avcodec_get_name(fmt->video_codec));
				return false;
			}
			if (open_video() == false) return false;
		}

		if (add_video_stream() == false) return false;

		if (fmt->audio_codec == AV_CODEC_ID_NONE) {
			DebugOutput::trace("createVideo - fmt->audio_codec == AV_CODEC_ID_NONE");
			return false;
		}
		
		if (add_audio_stream() == false) return false;
		if (open_audio() == false) return false;

		av_dump_format(oc, 0, option_.filename.c_str(), 1);

		if (!(fmt->flags & AVFMT_NOFILE)) {
			int ret = avio_open(&oc->pb, option_.filename.c_str(), AVIO_FLAG_WRITE);
			if (ret < 0) {
				DebugOutput::trace("createVideo - Could not open '%s'", option_.filename.c_str());
				return false;
			}
		}

		int ret = avformat_write_header(oc, NULL);
		if (ret < 0) {
			DebugOutput::trace("createVideo - avformat_write_header");
			return false;
		}

		is_opened_ = true;

		return true;
	}

	void close()
	{
		try {
			if (oc != nullptr) {
				if (is_opened_) av_write_trailer(oc);
				if (oc->pb != NULL) avio_closep(&oc->pb);
				avformat_free_context(oc);
				oc = nullptr;
			}

			close_stream(&video_st);
			close_stream(&audio_st);
		} catch (...) {
			DebugOutput::trace("VideoCreater.close()");
		}

		is_opened_ = false;
	}

	bool writeBitmap(void* bitmap)
	{
		AVFrame* frame = alloc_picture(codec_ctx_video->pix_fmt);
		AVFrame* tmp_frame = alloc_picture(AV_PIX_FMT_BGRA);
		if ((frame == nullptr) || (tmp_frame == nullptr)) {
			DebugOutput::trace("CreateVideo.writeBitmap - error alloc_picture(c->pix_fmt...)");
			return false;
		}

		AVPacket pkt = {0};
		av_init_packet(&pkt);
		BOOST_SCOPE_EXIT(&frame, &tmp_frame, &pkt)
		{
			av_frame_free(&frame);
			av_frame_free(&tmp_frame);
			av_packet_unref(&pkt);
		}
		BOOST_SCOPE_EXIT_END;

		if (av_frame_make_writable(frame) < 0) {
			DebugOutput::trace("writeBitmap - av_frame_make_writable < 0");
			return false;
		}

		memcpy(tmp_frame->data[0], bitmap, codec_ctx_video->width * codec_ctx_video->height * PIXEL_SIZE);
		sws_scale(video_st.sws_ctx, (const uint8_t* const*) tmp_frame->data,
			tmp_frame->linesize, 0, codec_ctx_video->height, frame->data,
			frame->linesize);

		frame->pts = video_st.next_pts++;

		int got_packet = 0;
		int ret = avcodec_encode_video2(codec_ctx_video, &pkt, frame, &got_packet);
		if (ret < 0) {
			DebugOutput::trace("Error encoding video frame");
			return false;
		}

		if (got_packet) {
			av_packet_rescale_ts(&pkt, codec_ctx_video->time_base, video_st.st->time_base);
			pkt.stream_index = video_st.st->index;
			ret = av_interleaved_write_frame(oc, &pkt);
			if (ret < 0) {
				DebugOutput::trace("Error while writing video frame");
				return false;
			}
		}

		return true;
	}

	bool do_writeAudioPacket(void* data, int size)
	{
		AVFrame* frame = alloc_audio_frame(codec_ctx_audio->sample_fmt, codec_ctx_audio->channel_layout, codec_ctx_audio->sample_rate);
		AVFrame* tmp_frame = alloc_audio_frame(AUDIO_FORMAT, option_.getCannelLayout(), option_.sample_rate);
		if ((frame == nullptr) || (tmp_frame == nullptr)) {
			DebugOutput::trace("CreateVideo.do_writeAudioPacket - error alloc_audio_frame(...)");
			return false;
		}

		AVPacket pkt = {0};
		av_init_packet(&pkt);
		BOOST_SCOPE_EXIT(&frame, &tmp_frame, &pkt)
		{
			av_frame_free(&frame);
			av_frame_free(&tmp_frame);
			av_packet_unref(&pkt);
		}
		BOOST_SCOPE_EXIT_END;

		memcpy(tmp_frame->data[0], data, size);
		tmp_frame->pts = audio_st.next_pts;
		audio_st.next_pts += tmp_frame->nb_samples;

		int dst_nb_samples = av_rescale_rnd(swr_get_delay(audio_st.swr_ctx, codec_ctx_audio->sample_rate) + tmp_frame->nb_samples, option_.sample_rate, codec_ctx_audio->sample_rate, AV_ROUND_UP);
		//DebugOutput::trace("audio_st.tmp_frame->nb_samples: %d, dst_nb_samples: %d", tmp_frame->nb_samples, dst_nb_samples);

		int ret = av_frame_make_writable(frame);
		if (ret < 0) {
			return false;
		}

		ret = swr_convert(audio_st.swr_ctx, frame->data, dst_nb_samples, (const uint8_t**) tmp_frame->data, tmp_frame->nb_samples);
		if (ret < 0) {
			DebugOutput::trace("Error while converting");
			return false;
		}

		AVRational time_base;
		time_base.num = 1;
		time_base.den = codec_ctx_audio->sample_rate;
		frame->pts = av_rescale_q(audio_st.samples_count, time_base, codec_ctx_audio->time_base);
		audio_st.samples_count += frame->nb_samples;

		int got_packet;
		ret = avcodec_encode_audio2(codec_ctx_audio, &pkt, frame, &got_packet);
		if (ret < 0) {
			DebugOutput::trace("Error encoding audio frame");
			return false;
		}

		if (got_packet) {
			av_packet_rescale_ts(&pkt, codec_ctx_audio->time_base, audio_st.st->time_base);
			pkt.stream_index = audio_st.st->index;
			ret = av_interleaved_write_frame(oc, &pkt);
			if (ret < 0) {
				DebugOutput::trace("Error while writing audio frame");
				return false;
			}
		}

		return true;
	}

	bool writeAudioPacket(void* packet, int size)
	{
		audio_buffer.write(packet, size);
		int data_size = audio_st.nb_samples * AUDIO_SAMPLE_SIZE * option_.channel;

		while (true) {
			void* data = audio_buffer.read(data_size);
			if (data == nullptr) break;

			if (do_writeAudioPacket(data, data_size) == false) {
				free(data);
				return false;
			}

			free(data);
		}

		return true;
	}

	bool isVideoTurn()
	{
		return av_compare_ts(
			video_st.next_pts,
			video_st.enc->time_base,
			audio_st.next_pts,
			audio_st.enc->time_base) <= 0;
	}

	bool isAudioTurn()
	{
		return !isVideoTurn();
	}

	int getBitmapWidth()
	{
		int result = option_.width;
		if ((result % BITMAP_CELL_WIDTH) != 0) result = result + (BITMAP_CELL_WIDTH - (result % BITMAP_CELL_WIDTH));
		return result;
	}

	// TODO: 중복 제거 및, 매직넘버 제거
	int getBitmapHeight()
	{
		int result = option_.height;
		if ((result % BITMAP_CELL_HEIGHT) != 0) result = result + (BITMAP_CELL_HEIGHT - (result % BITMAP_CELL_HEIGHT));
		return result;
	}

	int getBitmapSize() { return getBitmapWidth() * getBitmapHeight() * BITMAP_PIXEL_SIZE; }

private:
	VideoCreaterOption option_;

	bool is_opened_ = false;
	bool using_hw_encoder_ = true;
	
	AVOutputFormat* fmt = nullptr;
	AVFormatContext* oc = nullptr;

	AVCodecContext* codec_ctx_video = nullptr;
	OutputStream video_st;
	AVCodec* video_codec = nullptr;

	AVCodecContext* codec_ctx_audio = nullptr;
	OutputStream audio_st;
	AVCodec* audio_codec = nullptr;

	MemoryBuffer audio_buffer;

	AVFrame* alloc_audio_frame(enum AVSampleFormat sample_fmt, uint64_t channel_layout, int sample_rate)
	{
		AVFrame* frame = av_frame_alloc();
		int ret;

		if (!frame) {
			DebugOutput::trace("alloc_audio_frame - Error allocating an audio frame");
			return NULL;
		}

		frame->format = sample_fmt;
		frame->channel_layout = channel_layout;
		frame->sample_rate = sample_rate;
		frame->nb_samples = audio_st.nb_samples;

		if (audio_st.nb_samples) {
			ret = av_frame_get_buffer(frame, 0);
			if (ret < 0) {
				DebugOutput::trace("alloc_audio_frame - Error allocating an audio buffer");
				return NULL;
			}
		}

		return frame;
	}

	AVFrame* alloc_picture(enum AVPixelFormat pix_fmt)
	{
		AVFrame* picture = av_frame_alloc();
		if (!picture) {
			DebugOutput::trace("alloc_picture - Error allocating an video frame");
			return nullptr;
		}

		picture->format = pix_fmt;
		picture->width = codec_ctx_video->width;
		picture->height = codec_ctx_video->height;

		int ret = av_frame_get_buffer(picture, 32);
		if (ret < 0) {
			DebugOutput::trace("alloc_picture - Could not allocate frame data.\n");
			return nullptr;
		}

		return picture;
	}

	bool open_video()
	{
		codec_ctx_video = avcodec_alloc_context3(video_codec);
		if (!codec_ctx_video) {
			DebugOutput::trace("Could not alloc an encoding context");
			return false;
		}

		AVDictionary* opt = NULL;
		av_dict_copy(&opt, NULL, 0);
		if (using_hw_encoder_ == false) {
			av_opt_set(codec_ctx_video->priv_data, "preset", option_.speed.c_str(), 0);
			av_dict_set(&opt, "preset", option_.speed.c_str(), 0);
		}
		av_opt_set(codec_ctx_video->priv_data, "crf", "28", 0);
		av_dict_set(&opt, "crf", "28", 0);

		codec_ctx_video->time_base.num = 1;
		codec_ctx_video->time_base.den = option_.fps;
		codec_ctx_video->codec_id = fmt->video_codec;
		// codec_ctx_video->bit_rate = option_.bitrate;
		codec_ctx_video->width = getBitmapWidth();
		codec_ctx_video->height = getBitmapHeight();
		codec_ctx_video->gop_size = option_.gop_size;
		codec_ctx_video->pix_fmt = AV_PIX_FMT_YUV420P;
		//codec_ctx_video->flags |= AV_CODEC_FLAG_PASS2;
		//codec_ctx_video->qcompress = 1.0;
		//if (codec_ctx_video->codec_id == AV_CODEC_ID_MPEG2VIDEO) codec_ctx_video->max_b_frames = 2;
		//if (codec_ctx_video->codec_id == AV_CODEC_ID_MPEG1VIDEO) codec_ctx_video->mb_decision = 2;
		//if (oc->oformat->flags & AVFMT_GLOBALHEADER) codec_ctx_video->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

		int ret = avcodec_open2(codec_ctx_video, video_codec, &opt);
		av_dict_free(&opt);
		if (ret < 0) {
			DebugOutput::trace("Could not open video codec");
			return false;
		}

		if (video_st.sws_ctx != nullptr) sws_freeContext(video_st.sws_ctx);
		video_st.sws_ctx = sws_getContext(codec_ctx_video->width, codec_ctx_video->height,
			AV_PIX_FMT_BGRA,
			codec_ctx_video->width, codec_ctx_video->height,
			codec_ctx_video->pix_fmt,
			SWS_BICUBIC, NULL, NULL, NULL);
		if (!video_st.sws_ctx) {
			DebugOutput::trace("writeBitmap - Could not initialize the conversion context");
			return false;
		}

		return true;
	}

	bool add_video_stream()
	{
		video_st.st = avformat_new_stream(oc, NULL);
		if (!video_st.st) {
			DebugOutput::trace("Could not allocate stream");
			return false;
		}

		video_st.st->id = oc->nb_streams - 1;
		video_st.enc = codec_ctx_video;
		video_st.st->time_base = codec_ctx_video->time_base;

		int ret = avcodec_parameters_from_context(video_st.st->codecpar, codec_ctx_video);
		if (ret < 0) {
			DebugOutput::trace("Could not copy the stream parameters");
			return false;
		}

		return true;
	}

	bool add_audio_stream()
	{
		audio_codec = avcodec_find_encoder(fmt->audio_codec);
		if (!audio_codec) {
			DebugOutput::trace("Could not find encoder for '%s'", avcodec_get_name(fmt->audio_codec));
			return false;
		}

		audio_st.st = avformat_new_stream(oc, NULL);
		if (!audio_st.st) {
			DebugOutput::trace("Could not allocate stream");
			return false;
		}

		audio_st.st->id = oc->nb_streams - 1;
		codec_ctx_audio = avcodec_alloc_context3(audio_codec);
		if (!codec_ctx_audio) {
			DebugOutput::trace("Could not alloc an encoding context");
			return false;
		}

		audio_st.enc = codec_ctx_audio;
		codec_ctx_audio->sample_fmt = (audio_codec)->sample_fmts ? (audio_codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
		codec_ctx_audio->bit_rate = 64000;
		codec_ctx_audio->sample_rate = 44100;
		if ((audio_codec)->supported_samplerates) {
			codec_ctx_audio->sample_rate = (audio_codec)->supported_samplerates[0];
			for (int i = 0; (audio_codec)->supported_samplerates[i]; i++) {
				if ((audio_codec)->supported_samplerates[i] == 44100) codec_ctx_audio->sample_rate = 44100;
			}
		}
		codec_ctx_audio->channels = av_get_channel_layout_nb_channels(codec_ctx_audio->channel_layout);
		codec_ctx_audio->channel_layout = AV_CH_LAYOUT_STEREO;
		if ((audio_codec)->channel_layouts) {
			codec_ctx_audio->channel_layout = (audio_codec)->channel_layouts[0];
			for (int i = 0; (audio_codec)->channel_layouts[i]; i++) {
				if ((audio_codec)->channel_layouts[i] == AV_CH_LAYOUT_STEREO) codec_ctx_audio->channel_layout = AV_CH_LAYOUT_STEREO;
			}
		}
		codec_ctx_audio->channels = av_get_channel_layout_nb_channels(codec_ctx_audio->channel_layout);
		audio_st.st->time_base.num = 1;
		audio_st.st->time_base.den = codec_ctx_audio->sample_rate;

		if (oc->oformat->flags & AVFMT_GLOBALHEADER) codec_ctx_audio->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

		return true;
	}

	bool open_audio()
	{
		AVDictionary* opt = NULL;
		av_dict_copy(&opt, NULL, 0);
		int ret = avcodec_open2(codec_ctx_audio, audio_codec, &opt);
		av_dict_free(&opt);
		if (ret < 0) {
			DebugOutput::trace("Could not open audio codec");
			return false;
		}

		if (codec_ctx_audio->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE) {
			audio_st.nb_samples = 10000;
		} else {
			audio_st.nb_samples = codec_ctx_audio->frame_size;
		}

		ret = avcodec_parameters_from_context(audio_st.st->codecpar, codec_ctx_audio);
		if (ret < 0) {
			DebugOutput::trace("Could not copy the stream parameters");
			return false;
		}

		audio_st.swr_ctx = swr_alloc();
		if (!audio_st.swr_ctx) {
			DebugOutput::trace("Could not allocate resampler context");
			return false;
		}

		av_opt_set_int(audio_st.swr_ctx, "in_channel_count", option_.channel, 0);
		av_opt_set_int(audio_st.swr_ctx, "in_sample_rate", option_.sample_rate, 0);
		av_opt_set_sample_fmt(audio_st.swr_ctx, "in_sample_fmt", AUDIO_FORMAT, 0);
		av_opt_set_int(audio_st.swr_ctx, "out_channel_count", codec_ctx_audio->channels, 0);
		av_opt_set_int(audio_st.swr_ctx, "out_sample_rate", codec_ctx_audio->sample_rate, 0);
		av_opt_set_sample_fmt(audio_st.swr_ctx, "out_sample_fmt", codec_ctx_audio->sample_fmt, 0);

		if ((ret = swr_init(audio_st.swr_ctx)) < 0) {
			DebugOutput::trace("Failed to initialize the resampling context");
			return false;
		}

		return true;
	}

	void close_stream(OutputStream* ost)
	{
		if (ost->enc != nullptr) avcodec_free_context(&ost->enc);
		if (ost->sws_ctx != nullptr) sws_freeContext(ost->sws_ctx);
		if (ost->swr_ctx != nullptr) swr_free(&ost->swr_ctx);

		ost->enc = nullptr;
		ost->sws_ctx = nullptr;
		ost->swr_ctx = nullptr;
	}
};

#endif  // VIDEO_CREATER_HPP