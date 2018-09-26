#include"OpenH264Encode.h"
#include "svc/codec_api.h"
#include <memory>

namespace VideoEncoder
{
	OpenH264Encoder::OpenH264Encoder()
	{
		encoder_= nullptr;
		encode_param_ = new SEncParamExt;
		src_pic_ = new SSourcePicture;
	}

	OpenH264Encoder::~OpenH264Encoder()
	{
		Release();
		if (encode_param_)
		{
			delete encode_param_;
			encode_param_ = nullptr;
		}
		if (src_pic_)
		{
			delete src_pic_;
			src_pic_ = nullptr;
		}
	}

	bool OpenH264Encoder::Init(int src_w, int src_h, int fps, int bit_rate, bool temporal)
	{
		bool ret = false;
		if (init_encoder_)
		{
			if (encode_param_->iPicWidth == src_w&&encode_param_->iPicHeight == src_h&& temporal == temporal_)
			{

			}
		}
		Release();
		int err = WelsCreateSVCEncoder(&encoder_);
		if (cmResultSuccess != err)
		{
			Release();
			return ret;
		}
		temporal_ = temporal;
		encoder_->GetDefaultParams(encode_param_);
		
		ECOMPLEXITY_MODE complexityMode = HIGH_COMPLEXITY;
		RC_MODES rc_mode = RC_QUALITY_MODE;

		encode_param_->iPicWidth = src_w;
		encode_param_->iPicHeight = src_h;
		encode_param_->iTargetBitrate = bit_rate;
		encode_param_->iMaxBitrate = 6000000;
		
		//时域SVC的层数
		encode_param_->iTemporalLayerNum = 4;

		encode_param_->bEnableBackgroundDetection = 1;  //背景检测，主要用于VaaCalculation和CalculateBGD，用于BGD control，统计分析图像复杂度
		encode_param_->bEnableSceneChangeDetect = 1;	//场景变换检测，当检测到场景发生变换时，会插入I帧
		encode_param_->bEnableAdaptiveQuant = false;	//自适应量化控制
		//以上三个参数对应的算法都是预处理过程，在编码前对视频帧进行一些检测，并利用检测算法得到的特征调控编码器的算法，影响编码性能。该算法会增加复杂度，但是也会改善编码性能，默认都是开启

		encode_param_->iRCMode = rc_mode;
		//encode_param_->fMaxFrameRate=
		encode_param_->iComplexityMode = complexityMode;
		encode_param_->iNumRefFrame = -1;
		encode_param_->bEnableFrameSkip = false;
		encode_param_->eSpsPpsIdStrategy = CONSTANT_ID;

		//熵编码策略
		encode_param_->iEntropyCodingModeFlag = 0;// 0:CAVLC  1:CABAC. 生命期是SLICE
		encode_param_->bEnableSSEI = true;

		SSpatialLayerConfig* spaticalLayerCfg = &(encode_param_->sSpatialLayers[SPATIAL_LAYER_0]);
		spaticalLayerCfg->iVideoWidth = src_w;
		spaticalLayerCfg->iVideoHeight = src_h;
		spaticalLayerCfg->fFrameRate = bit_rate;
		spaticalLayerCfg->iSpatialBitrate = encode_param_->iTargetBitrate;
		spaticalLayerCfg->iMaxSpatialBitrate = encode_param_->iMaxBitrate;

		//spaticalLayerCfg->sSliceArgument.uiSliceMode = SM_SINGLE_SLICE;

		if (cmResultSuccess != (err = encoder_->InitializeExt(encode_param_)))
		{
			Release();
			return ret;
		}

		//设置日志级别
		int log_level = WELS_LOG_ERROR;
		encoder_->SetOption(ENCODER_OPTION_TRACE_LEVEL, &log_level);

		memset(src_pic_, 0, sizeof(SSourcePicture));
		src_pic_->iPicWidth = src_w;
		src_pic_->iPicHeight = src_h;
		src_pic_->iColorFormat = videoFormatI420;
		src_pic_->iStride[0] = src_pic_->iPicWidth;
		src_pic_->iStride[1] = src_pic_->iStride[2] = src_pic_->iPicWidth >> 1;
		//yuvData = CFDataCreateMutable(kCFAllocatorDefault, inputSize.width * inputSize.height * 3 >> 1);
		pic_data_temp_ = new char[src_pic_->iPicWidth * src_pic_->iPicHeight * 3 >> 1];
		src_pic_->pData[0] = (unsigned char*)pic_data_temp_;//CFDataGetMutableBytePtr(yuvData);
		src_pic_->pData[1] = src_pic_->pData[0] + src_pic_->iPicWidth * src_pic_->iPicHeight;
		src_pic_->pData[2] = src_pic_->pData[1] + (src_pic_->iPicWidth * src_pic_->iPicHeight >> 2);

		init_encoder_ = true;
		ret = true;
		return ret;
	}

	bool OpenH264Encoder::Release()
	{
		if (encoder_)
		{
			encoder_->Uninitialize();
			WelsDestroySVCEncoder(encoder_);
			encoder_ = nullptr;
		}
		if (pic_data_temp_)
		{
			delete[] pic_data_temp_;
			pic_data_temp_ = nullptr;
		}
		init_encoder_ = false;
	}

	bool OpenH264Encoder::Encode(const char* in_data, unsigned __int64 time, char* out_data, int& temporal_id, int& frame_type)
	{
		bool ret = false;
		if (!encoder_)
		{
			return ret;
		}

		src_pic_->uiTimeStamp = time;
		int size_buffer = src_pic_->iPicWidth * src_pic_->iPicHeight * 3 >> 1;
		memcpy(src_pic_->pData[0], in_data, size_buffer);

		int iFrameSize = 0;
		SFrameBSInfo encoded_frame_info;
		int err = encoder_->EncodeFrame(src_pic_, &encoded_frame_info);
		if (err)
		{
			Release();
			return ret;
		}
		if (encoded_frame_info.eFrameType == videoFrameTypeInvalid)
		{
			return ret;
		}

		if (encoded_frame_info.eFrameType != videoFrameTypeSkip) 
		{
			int iLayer = 0;
			//将编码数据放置在输出缓存中
			while (iLayer < encoded_frame_info.iLayerNum) 
			{
				SLayerBSInfo* pLayerBsInfo = &(encoded_frame_info.sLayerInfo[iLayer]);
				if (pLayerBsInfo != NULL) 
				{
					int iLayerSize = 0;
					temporal_id = pLayerBsInfo->uiTemporalId;
					int iNalIdx = pLayerBsInfo->iNalCount - 1;
					do {
						iLayerSize += pLayerBsInfo->pNalLengthInByte[iNalIdx];
						--iNalIdx;
					} while (iNalIdx >= 0);
					memcpy(out_data + iFrameSize, pLayerBsInfo->pBsBuf, iLayerSize);
					iFrameSize += iLayerSize;
				}
				++iLayer;
			}
		}
		else
		{
			//产生了跳帧
		}

		if (iFrameSize > 0)
		{
			frame_type = encoded_frame_info.eFrameType;
		}
		return iFrameSize;
	}

// end namespace
}
