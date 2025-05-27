sudo apt-get install libmosquitto-dev
git clone https://github.com/iwishiwasaneagle/wiringPi-mock.git
cd wiringPi-mock
sudo ./install.sh
cd ..


git clone https://github.com/anonim228228/robot.git
cd robot
g++ robot.cpp -o robot -lwiringPi -lmosquitto -I/opt/wiringPi-mock/include -L/opt/wiringPi-mock/lib
./robot
