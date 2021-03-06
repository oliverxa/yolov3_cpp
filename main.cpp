#include <torch/torch.h>
#include <iostream>
#include <chrono>
#include <time.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "Darknet.h"

using namespace std;
using namespace std::chrono;

void yolov3(string path);

int main(int argc, const char* argv[])
{
    std::cerr << "usage: yolo-app <image path>\n";
    yolov3(argv[1]);
    return 0;
}

void yolov3(string path){

    torch::DeviceType device_type;

    if (torch::cuda::is_available() ) {
        device_type = torch::kCUDA;
        std::cout << "GPU--version"<< endl;
    } else {
        device_type = torch::kCPU;
        std::cout << "CPU--version"<< endl;
    }
    torch::Device device(device_type);

    // input image size for YOLO v3
    int input_image_size = 416;

    Darknet net("../models/yolov3-swim-test.cfg", &device);

    map<string, string> *info = net.get_net_info();

    info->operator[]("height") = std::to_string(input_image_size);

    std::cout << "loading weight ..." << endl;
    net.load_weights("../models/yolov3-swim_final.weights");
    std::cout << "weight loaded ..." << endl;

    net.to(device);

    torch::NoGradGuard no_grad;
    net.eval();

    std::cout << "start to inference ..." << endl;

    // origin_image = cv::imread("../139.jpg");
    //origin_image = cv::imread(path);
    cv::VideoCapture capture(path);

    if(!capture.isOpened()){
        std::cout << "Read video Failed" << std::endl;
        return;
    }

    cv::Mat origin_image, resized_image;
    //cv::namedWindow("video test");
    int frame_num = capture.get(cv::CAP_PROP_FRAME_COUNT);

    for(int i = 0; i < frame_num; i++){

        capture >> origin_image;
        //imshow("video test", origin_image);

        cv::cvtColor(origin_image, resized_image,  cv::COLOR_BGR2RGB);
        cv::resize(resized_image, resized_image, cv::Size(input_image_size, input_image_size));

        cv::Mat img_float;
        resized_image.convertTo(img_float, CV_32F, 1.0/255);

        auto img_tensor = torch::from_blob(img_float.data, {1, input_image_size, input_image_size, 3}).to(device);
        img_tensor = img_tensor.permute({0,3,1,2});

        auto start = std::chrono::high_resolution_clock::now();

        auto output = net.forward(img_tensor);

        // filter result by NMS
        // class_num = 80
        // confidence = 0.6
        auto result = net.write_results(output, 80, 0.6, 0.4);

        auto end = std::chrono::high_resolution_clock::now();

        auto duration = duration_cast<milliseconds>(end - start);

        // It should be known that it takes longer time at first time
        std::cout << "inference taken : " << duration.count() << " ms" << endl;

        if (result.dim() == 1)
        {
            std::cout << "no object found" << endl;
        }
        else
        {
            int obj_num = result.size(0);

            std::cout << obj_num << " objects found" << endl;

            float w_scale = float(origin_image.cols) / input_image_size;
            float h_scale = float(origin_image.rows) / input_image_size;

            result.select(1,1).mul_(w_scale);
            result.select(1,2).mul_(h_scale);
            result.select(1,3).mul_(w_scale);
            result.select(1,4).mul_(h_scale);

            auto result_data = result.accessor<float, 2>();

            // xmin, ymin, xmax, ymax
            for (int i = 0; i < result.size(0) ; i++)
            {
                cv::rectangle(origin_image, cv::Point(result_data[i][1], result_data[i][2]), cv::Point(result_data[i][3], result_data[i][4]), cv::Scalar(0, 0, 255), 1, 1, 0);
                std::cout << result_data[i][1] << ' ' << result_data[i][2] << ' ' << result_data[i][3] << ' ' << result_data[i][4] << '\n' ;
            }

            //cv::imwrite("out-det.jpg", origin_image);

        }
        cv::Mat output_image;
        cv::resize(origin_image, output_image, cv::Size(600, 600));
        cv::imshow("video test", output_image);
        if( cv::waitKey(10) == 27 ) break;

    }
    cv::destroyWindow("video test");
	capture.release();
    std::cout << "Done" << endl;

    return;
}
