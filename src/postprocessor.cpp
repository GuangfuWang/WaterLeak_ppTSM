#include <opencv2/imgproc.hpp>
#include "postprocessor.h"
#include "config.h"

namespace waterleak_pptsm {

    void PpTSMDeployPost::Run(const SharedRef<TrtResults> &res, const std::vector<cv::Mat> &img,
							  std::vector<cv::Mat> &out_img, int & alarm) {
        //our simple program will only draw letters on top of images.
		alarm = 0;
        auto flag = static_cast<PostProcessFlag>(m_config->POST_MODE);
        std::vector<float> waterleak_res;
        res->Get(m_config->OUTPUT_NAMES[0], waterleak_res);
        Util::softmax(waterleak_res);

        m_moving_average.push_back(waterleak_res[m_config->TARGET_CLASS]);

        float sum=0.0f;
        if (m_moving_average.size()>3){
            for(auto i = m_moving_average.size()-1;i>=m_moving_average.size()-3;--i){
                sum+=m_moving_average[i];
            }
            sum/=3.0f;
			auto it = m_moving_average.begin();
			m_moving_average.erase(it);
        }
//		std::cout<<"Curr Frame: "<<sum<<std::endl;
		if(sum>=m_config->THRESHOLD){
			alarm = 1;
//			std::cout<<"RAW: "<<waterleak_res[m_config->TARGET_CLASS]<<std::endl;
		}
    }

    void Postprocessor::Run(const SharedRef<TrtResults> &res, const std::vector<cv::Mat> &img,
                            std::vector<cv::Mat> &out_img,int& alarm) {
        if (!INIT_FLAG) {
            m_worker = new PpTSMDeployPost(m_config);
            INIT_FLAG = true;
        }
        m_worker->Run(res, img, out_img,alarm);
    }

    Postprocessor::~Postprocessor() {
		if(m_worker){
			delete m_worker;
			m_worker = nullptr;
		}
    }

}
