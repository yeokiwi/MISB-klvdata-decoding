// misb_decode.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <iostream>
#include <iomanip>
#include <string>
#include <map>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <opencv2/opencv.hpp>

extern "C" {
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>

}
double parse_double_from_bytes(const uint8_t* data, int len, double min_val, double max_val) {
    uint64_t raw = 0;
    for (int i = 0; i < len; ++i)
        raw = (raw << 8) | data[i];
    double max_raw = (1ULL << (len * 8)) - 1;
    return min_val + ((double)raw / max_raw) * (max_val - min_val);
}

uint64_t parse_uint64_from_bytes(const uint8_t* data, int len) {
    uint64_t raw = 0;
    for (int i = 0; i < len; ++i)
        raw = (raw << 8) | data[i];
    
    return raw;
}

std::string utcMicrosecondsToUnixTimeString(uint64_t utc_microseconds) {
    time_t seconds = static_cast<time_t>(utc_microseconds / 1000000);
    struct tm timeinfo;

#if defined(_WIN32) || defined(_WIN64)
    if (gmtime_s(&timeinfo, &seconds) != 0) {
        return "Invalid time";
    }
#else
    if (gmtime_r(&seconds, &timeinfo) == nullptr) {
        return "Invalid time";
    }
#endif

    char buffer[30];
    if (strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo)) {
        return std::string(buffer);
    }
    else {
        return "Invalid time format";
    }
}
std::string parse_string(const uint8_t* data, int len) {
    return std::string(data, data + len);
}

void parse_klv_payload(const uint8_t* data, int size) {
    int pos = 0;
    if (size < 16) {
        std::cout << "KLV packet too small\n";
        return;
    }

    // UDS Key check
    const uint8_t klv_key_prefix[4] = { 0x06, 0x0E, 0x2B, 0x34 };
    if (memcmp(data, klv_key_prefix, 4) != 0) {
        std::cout << "Invalid KLV Key\n";
        return;
    }

    // Skip 16-byte Universal Key
    pos += 16;

    int len;
    // Read BER length (only 1-byte for most practical cases)
    if ((data[pos] & 0x80) == 0x80)
    {
        pos++;
        len = data[pos++];
    }
    else
    {
        len = data[pos++];
    }
    if (len > size - pos) {
        std::cout << "Invalid BER length\n";
        return;
    }

    // Start reading tag-length-value triplets
    while (pos < size) {
        uint8_t tag = data[pos++];
        if (pos >= size) break;
        uint8_t length = data[pos++];
        if (pos + length > size) break;

        const uint8_t* value = &data[pos];

        switch (tag) {
        case 2: { // Precision time Stamp
            uint64_t time = parse_uint64_from_bytes(value, length);
            std::string unix_time = utcMicrosecondsToUnixTimeString(time);
            std::cout << "Tag 2 (Platform Designation): " << unix_time << "\n";
            break;
        }
        case 65: { // UAS Datalink LS version
            uint64_t ls_version = parse_uint64_from_bytes(value, length);
            std::cout << "Tag 65 (UAS Datalink LS version): " << ls_version << "\n";
            break;
       }
        case 5: { // Platform Heading Angle
            double angle = parse_double_from_bytes(value, length, 0.0, 360.0);
            std::cout << "Tag 5 (Platform Heading): " << std::fixed << std::setprecision(2) << angle << " deg\n";
            break;
        }
        case 6: { // Platform Pitch Angle
            double angle = parse_double_from_bytes(value, length, 0.0, 360.0);
            std::cout << "Tag 6 (Platform Pitch): " << std::fixed << std::setprecision(2) << angle << " deg\n";
            break;
        }
        case 7: { // Platform Roll Angle
            double angle = parse_double_from_bytes(value, length, 0.0, 360.0);
            std::cout << "Tag 7 (Platform Roll): " << std::fixed << std::setprecision(2) << angle << " deg\n";
            break;
        }
        case 11: { // Image Source Sensor
            std::string source = parse_string(value, length);
            std::cout << "Tag 11 (Image Source Sensor): " << source << "\n";
            break;
        }
        case 12: { // Image coordinate system
            std::string source = parse_string(value, length);
            std::cout << "Tag 12 (Image coordinate system): " << source << "\n";
            break;
        }
        case 13: { // Sensor Latitude
            double lat = parse_double_from_bytes(value, length, -90.0, 90.0);
            std::cout << "Tag 13 (Sensor Latitude): " << lat << "\n";
            break;
        }
        case 14: { // Sensor Longitude
            double lon = parse_double_from_bytes(value, length, -180.0, 180.0);
            std::cout << "Tag 14 (Sensor Longitude): " << lon << "\n";
            break;
        }
        case 15: { // Sensor True Altitude
            double alt = parse_double_from_bytes(value, length, -900.0, 19000.0);
            std::cout << "Tag 15 (Sensor Altitude): " << alt << " meters\n";
            break;
        }
        case 16: { // Sensor Horizontal Field of View
            double fov = parse_double_from_bytes(value, length, 0.0, 360.0);
            std::cout << "Tag 16 (Sensor Horizontal Field of View): " << fov << " deg\n";
            break;
        }
        case 17: { // Sensor vertical Field of View
            double fov = parse_double_from_bytes(value, length, 0.0, 360.0);
            std::cout << "Tag 17 (Sensor vertical Field of View): " << fov << " deg\n";
            break;
        }
        case 18: { // Sensor Relative Azimuth Angle
            double fov = parse_double_from_bytes(value, length, 0.0, 360.0);
            std::cout << "Tag 18 (Sensor Relative Azimuth Angle): " << fov << " deg\n";
            break;
        }
        case 19: { // Sensor Relative Elevation Angle
            double fov = parse_double_from_bytes(value, length, 0.0, 360.0);
            std::cout << "Tag 19 (Sensor Relative Elevation Angle): " << fov << " deg\n";
            break;
        }
        case 20: { // Sensor Relative Roll Angle
            double fov = parse_double_from_bytes(value, length, 0.0, 360.0);
            std::cout << "Tag 20 (Sensor Relative Roll Angle): " << fov << " deg\n";
            break;
        }
        case 21: { // Slant Range
            double slant = parse_double_from_bytes(value, length, 0.0, 360.0);
            std::cout << "Tag 21 (Slant Range): " << slant << " metres\n";
            break;
        }
        default:
            std::cout << "Tag " << (int)tag << ": (length=" << (int)length << ") ";
            for (int i = 0; i < length; ++i) printf("%02X ", value[i]);
            std::cout << "\n";
            break;
        }

        pos += length;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <udp_stream_url>\n";
        return -1;
    }

    
    const char* url = argv[1];
    avformat_network_init();
    printf("init\n");
    AVFormatContext* fmt_ctx = nullptr;

    if (avformat_open_input(&fmt_ctx, url, nullptr, nullptr) < 0) {
        std::cerr << "Could not open UDP stream: " << url << "\n";
        return -1;
    }
    printf("init udp\n");

    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        std::cerr << "Could not find stream information\n";
        avformat_close_input(&fmt_ctx);
        return -1;
    }
    printf("init stream\n");

    int klv_stream_index = -1;
    for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_DATA &&
            fmt_ctx->streams[i]->codecpar->codec_id == AV_CODEC_ID_SMPTE_KLV) {
            klv_stream_index = i;
            break;
        }
    }

    if (klv_stream_index == -1) {
        std::cerr << "No KLV stream found\n";
        avformat_close_input(&fmt_ctx);
        return -1;
    }

    std::cout << "KLV stream found at index " << klv_stream_index << "\n";

    
    AVPacket* pkt = av_packet_alloc();

    // Find the video stream
    int video_stream_index = -1;
    for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            printf("video stream index %d\n", i);
            break;
        }
    }

    if (video_stream_index == -1) {
        std::cerr << "No video stream found." << std::endl;
        return -1;
    }

    AVCodecParameters* codecpar = fmt_ctx->streams[video_stream_index]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        std::cerr << "Decoder not found." << std::endl;
        return -1;
    }

    AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        std::cerr << "Failed to allocate codec context." << std::endl;
        return -1;
    }

    avcodec_parameters_to_context(codec_ctx, codecpar);
    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
        std::cerr << "Failed to open codec." << std::endl;
        return -1;
    }

    // Allocate frames and packet
    AVFrame* frame = av_frame_alloc();
    AVFrame* rgb_frame = av_frame_alloc();
//    AVPacket* packet = av_packet_alloc();

    int width = codec_ctx->width;
    int height = codec_ctx->height;
    int num_bytes = av_image_get_buffer_size(AV_PIX_FMT_BGR24, width, height, 1);
    std::vector<uint8_t> buffer(num_bytes);
    av_image_fill_arrays(rgb_frame->data, rgb_frame->linesize, buffer.data(), AV_PIX_FMT_BGR24, width, height, 1);

    SwsContext* sws_ctx = sws_getContext(width, height, codec_ctx->pix_fmt,
        width, height, AV_PIX_FMT_BGR24,
        SWS_BILINEAR, nullptr, nullptr, nullptr);


    while (av_read_frame(fmt_ctx, pkt) >= 0) {
        if (pkt->stream_index == klv_stream_index) {
            parse_klv_payload(pkt->data, pkt->size);
            std::cout << "-----------------------------\n";
        }
    
        if (pkt->stream_index == video_stream_index) {
            if (avcodec_send_packet(codec_ctx, pkt) == 0) {
                while (avcodec_receive_frame(codec_ctx, frame) == 0) {
                    // Convert to BGR for OpenCV
                    sws_scale(sws_ctx, frame->data, frame->linesize, 0, height,
                        rgb_frame->data, rgb_frame->linesize);

                    cv::Mat img(height, width, CV_8UC3, rgb_frame->data[0], rgb_frame->linesize[0]);
                    cv::imshow("FFmpeg + OpenCV", img);
                    if (cv::waitKey(30) == 27) break; // ESC to exit
                }
            }
        }
        av_packet_unref(pkt);

    
    }
    // Clean up
    sws_freeContext(sws_ctx);
    av_frame_free(&frame);
    av_frame_free(&rgb_frame);
    av_packet_free(&pkt);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);
    avformat_network_deinit();

    avformat_close_input(&fmt_ctx);
    avformat_network_deinit();
    return 0;
}
