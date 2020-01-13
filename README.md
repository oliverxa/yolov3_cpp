# yolov3_cpp

# download yolov3 model
cd models 

wget https://pjreddie.com/media/files/yolov3.weights 

# build cmake for cpp

mkdir build

cd build

cmake ..

make

# run video
mkdir videos

add your videos (in videos path)

in build path run :

./yolo-app ../videos
