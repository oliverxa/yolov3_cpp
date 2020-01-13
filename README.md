# yolov3_cpp
cd models 

wget https://pjreddie.com/media/files/yolov3.weights 

mkdir build
cd build
cmake ..
make

./yolo-app ../imgs/person.jpg
