/*!
 *  Copyright (c) 2019 by Contributors if not otherwise specified
 * \file video_reader.h
 * \brief FFmpeg video reader, implements VideoReaderInterface
 */

#ifndef DECORD_VIDEO_VIDEO_READER_H_
#define DECORD_VIDEO_VIDEO_READER_H_

#include "threaded_decoder_interface.h"
#include "storage_pool.h"
#include <decord/video_interface.h>

#include <string>
#include <vector>

#include <decord/base.h>
#include <dmlc/concurrency.h>


namespace decord {
using timestamp_t = float;
struct AVFrameTime {
    int64_t pts;          // presentation timestamp, unit is stream time_base
    int64_t dts;          // decoding timestamp, unit is stream time_base
    timestamp_t start;    // real world start timestamp, unit is second
    timestamp_t stop;     // real world stop timestamp, unit is second

    AVFrameTime(int64_t pts=AV_NOPTS_VALUE, int64_t dts=AV_NOPTS_VALUE, timestamp_t start=0, timestamp_t stop=0)
     : pts(pts), dts(dts), start(start), stop(stop) {}
};  // struct AVFrameTime

class VideoReader : public VideoReaderInterface {
    using ThreadedDecoderPtr = std::unique_ptr<ThreadedDecoderInterface>;
    using NDArray = runtime::NDArray;
    public:
        VideoReader(std::string fn, DLContext ctx, int width=-1, int height=-1,
                    int nb_thread=0, int io_type=kNormal, std::string fault_tol="-1",
                    bool use_rrc=false,
                    double scale_min=0.08, double scale_max=1.,
                    double ratio_min=0.75, double ratio_max=4./3,
                    bool use_msc=false,
                    bool use_rcc=false,
                    bool use_centercrop=false,
                    bool use_fixedcrop=false, int crop_x=0, int crop_y=0,
                    double hflip_prob=0., double vflip_prob=0.);
        /*! \brief Destructor, note that FFMPEG resources has to be managed manually to avoid resource leak */
        ~VideoReader();
        void SetVideoStream(int stream_nb = -1);
        unsigned int QueryStreams() const;
        int64_t GetFrameCount() const;
        int64_t GetCurrentPosition() const;
        NDArray NextFrame();
        NDArray GetBatch(std::vector<int64_t> indices, NDArray buf);
        void SkipFrames(int64_t num = 1);
        bool Seek(int64_t pos);
        bool SeekAccurate(int64_t pos);
        NDArray GetKeyIndices();
        NDArray GetFramePTS() const;
        double GetAverageFPS() const;
        double GetRotation() const;
    protected:
        friend class VideoLoader;
        std::vector<int64_t> GetKeyIndicesVector() const;
    private:
        void IndexKeyframes();
        void PushNext();
        int64_t LocateKeyframe(int64_t pos);
        void SkipFramesImpl(int64_t num = 1);
        bool CheckKeyFrame();
        NDArray NextFrameImpl();
        int64_t FrameToPTS(int64_t pos);
        std::vector<int64_t> FramesToPTS(const std::vector<int64_t>& positions);
        void CacheFrame(NDArray frame);
        bool FetchCachedFrame(NDArray &frame, int64_t pos);

        DLContext ctx_;
        std::vector<int64_t> key_indices_;
        std::map<int64_t, int64_t> pts_frame_map_;
        NDArray tmp_key_frame_;
        bool overrun_;
        /*! \brief a lookup table for per frame pts/dts */
        std::vector<AVFrameTime> frame_ts_;
        /*! \brief Video Streams Codecs in original videos */
        std::vector<const AVCodec*> codecs_;
        /*! \brief Currently active video stream index */
        int actv_stm_idx_;
        /*! \brief AV format context holder */
        ffmpeg::AVFormatContextPtr fmt_ctx_;
        ThreadedDecoderPtr decoder_;
        int64_t curr_frame_;  // current frame location
        int64_t nb_thread_decoding_;  // number of threads for decoding
        int width_;   // output video width
        int height_;  // output video height
        bool eof_;  // end of file indicator
        NDArrayPool ndarray_pool_;
        std::unique_ptr<ffmpeg::AVIOBytesContext> io_ctx_;  // avio context for raw memory access
        std::string filename_;  // file name if from file directly, can be empty if from bytes
        NDArray cached_frame_;  // last valid frame, for error tolerance
        bool use_cached_frame_;  // switch to enable frame recovery if failed to decode
        std::unordered_set<int64_t> failed_idx_;  // idx of failed frames(recovered from other frames)
        int64_t fault_tol_thresh_;  // fault tolerance threshold, raise if recovered frames retrieved exceeds thresh
        bool fault_warn_emit_;  // whether a fault warning has been emitted

        bool use_rrc_;
        double scale_min_;
        double scale_max_;
        double ratio_min_;
        double ratio_max_;

        bool use_msc_;
        bool use_rcc_;

        bool use_centercrop_;

        bool use_fixedcrop_;
        int crop_x_;
        int crop_y_;

        double hflip_prob_;
        double vflip_prob_;
};  // class VideoReader
}  // namespace decord
#endif  // DECORD_VIDEO_VIDEO_READER_H_
