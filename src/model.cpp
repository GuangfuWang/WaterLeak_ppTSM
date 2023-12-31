#include "model.h"
#include "config.h"
#include "trt_deploy.h"
#include "trt_deployresult.h"
#include <opencv2/freetype.hpp>

namespace waterleak_pptsm
{


class InferModel
{
public:
	explicit InferModel(SharedRef<Config> &config)
	{
		m_config = config;
		mDeploy = createSharedRef<TrtDeploy>(config);
		mResult = createSharedRef<TrtResults>(config);

		if (!Util::checkFileExist(config->POST_TEXT_FONT_FILE))
			std::cerr << "Font file not found!" << std::endl;
		else {
			m_font = cv::freetype::createFreeType2();
			m_font->loadFontData(config->POST_TEXT_FONT_FILE, 0);
		}
	}

public:
	SharedRef<TrtDeploy> mDeploy;
	SharedRef<TrtResults> mResult;
	std::vector<cv::Mat> mSampled;
	std::vector<cv::Mat> mTotal;
	SharedRef<Config> m_config;
	cv::Mat m_roi;
	unsigned int COUNT_LOOP = 0;
	unsigned int COUNT = 0;
	unsigned int COUNT_TOTAL = 0;
	int latency = 0;
	cv::Ptr<cv::freetype::FreeType2> m_font = nullptr;
};

void *GenModel(SharedRef<Config> &config)
{
	auto *model = new InferModel(config);
	return reinterpret_cast<void *>(model);
}

cv::Mat genROI(const cv::Size &s, const std::vector<int> &points, const cv_Point *coords)
{
	cv::Mat roi_img = cv::Mat::zeros(s, CV_8UC3);
	if (points.empty())return cv::Mat(s, CV_8UC3, cv::Scalar(255, 255, 255));
	std::vector<std::vector<cv::Point>> contour;

	int sums = 0;
	for (auto &each : points) {
		std::vector<cv::Point> pts;
		for (int j = sums; j < each + sums; ++j) {
			pts.push_back(cv::Point(coords[j].x, coords[j].y));
		}
		sums += each;
		contour.push_back(pts);
	}
	sums = 0;
	for (auto &i : points) {
		cv::drawContours(roi_img, contour, sums,
						 cv::Scalar::all(255), -1);
		sums++;
	}
	return roi_img;
}

void plotLines(cv::Mat &im, const std::vector<int> &points,
			   const cv_Point *coords, const int &thickness)
{
	int sums = 0;
	for (auto &each : points) {
		for (int j = sums; j < each + sums; ++j) {
			int k = j + 1;
			if (k == each + sums)k = sums;
			cv::line(im, cv::Point(coords[j].x, coords[j].y),
					 cv::Point(coords[k].x, coords[k].y), cv::Scalar(255, 0, 0),
					 thickness);
		}
		sums += each;
	}
}

cvModel *Allocate_Algorithm(cv::Mat &input_frame, int algID, int gpuID)
{
	cv::cuda::setDevice(gpuID);
	cudaSetDevice(gpuID);
	std::string file;
	if (Util::checkFileExist("./waterleak_pptsm.yaml"))
		file = "./waterleak_pptsm.yaml";
	else if (Util::checkFileExist("../config/waterleak_pptsm.yaml")) {
		file = "../config/waterleak_pptsm.yaml";
	}
	else {
		std::cerr << "Cannot find YAML file!" << std::endl;
	}
	auto config = createSharedRef<Config>(0, nullptr, file);
	auto *ptr = new cvModel();
	ptr->FrameNum = 0;
	ptr->Frameinterval = 0;
	ptr->countNum = 0;
	ptr->width = input_frame.cols;
	ptr->height = input_frame.rows;
	ptr->iModel = GenModel(config);
	auto model = reinterpret_cast<InferModel *>(ptr->iModel);
	model->COUNT = config->SAMPLE_INTERVAL * config->TRIGGER_LEN;
	return ptr;
}

void SetPara_Algorithm(cvModel *pModel, int algID)
{
	//todo: implement this
}

void UpdateParams_Algorithm(cvModel *pModel)
{
	auto model = reinterpret_cast<InferModel *>(pModel->iModel);
	auto roi = pModel->p;
	cv::Size s(pModel->width, pModel->height);
	model->m_roi = genROI(s, pModel->pointNum, roi);
}

void Process_Algorithm(cvModel *pModel, cv::Mat &input_frame)
{
	pModel->alarm = 0;
	auto model = reinterpret_cast<InferModel *>(pModel->iModel);
	auto config = model->m_config;
	auto roi = pModel->p;
	if (model->m_roi.empty()) {
		model->m_roi = genROI(input_frame.size(), pModel->pointNum, roi);
	}

	cv::Mat removed_roi;
	input_frame.copyTo(removed_roi, model->m_roi);

	if (model->COUNT_LOOP < model->COUNT) {
		if ((model->COUNT_TOTAL % config->SAMPLE_INTERVAL) == 0) {
			model->mSampled.push_back(removed_roi.clone());

		}
		if (config->POST_MODE != 4)model->mTotal.push_back(input_frame.clone());
		model->COUNT_LOOP++;
	}
	else {
		model->mDeploy->Infer(model->mSampled, model->mResult);
		model->mDeploy->Postprocessing(model->mResult, model->mSampled, model->mTotal, pModel->alarm);
		if (pModel->alarm)model->latency = 10;
		model->mSampled.clear();
		model->mSampled.push_back(removed_roi.clone());
		model->COUNT_LOOP = 1;
		if (config->POST_MODE != 4) {
			model->mTotal.clear();
			model->mTotal.push_back(input_frame.clone());
		}
	}
	if (pModel->alarm || model->latency) {
		if (model->m_font) {
			model->m_font->putText(input_frame, config->POST_TEXT,
								   cv::Point(config->TEXT_OFF_X, config->TEXT_OFF_Y),
								   config->TEXT_FONT_SIZE,
								   cv::Scalar(config->TEXT_COLOR[0],
											  config->TEXT_COLOR[1],
											  config->TEXT_COLOR[2]),
								   (int)config->TEXT_LINE_WIDTH,
								   8, false);
		}
		model->latency--;
		if (model->latency == 0)model->latency = 0;
	}
	model->COUNT_TOTAL++;
	plotLines(input_frame, pModel->pointNum,
			  roi, (int)config->BOX_LINE_WIDTH);
}

void Destroy_Algorithm(cvModel *pModel)
{
	if (pModel->iModel) {
		auto model = reinterpret_cast<InferModel *>(pModel->iModel);
		delete model;
		model = nullptr;
	}
	if (pModel) {
		delete pModel;
		pModel = nullptr;
	}
}

} // namespace waterleak_pptsm
