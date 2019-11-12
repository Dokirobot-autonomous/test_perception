//位置(X,Y,θ,v)

#pragma once

#define _USE_MATH_DEFINES
#include <cmath>
#include <sstream>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>

template<typename T = double>class State{
public:
    T x;	//座標 x
    T y;	//座標 y
    T r;	//角度θ[rad]
    T vx;
    T vy;
    size_t size=5;

    //コンストラクタ
    State() :x(0), y(0), r(0), vx(0), vy(0){}
    State(T x, T y, T r, T vx, T vy) :x(x), y(y), r(r), vx(vy), vy(vy){}
    //コピーコンストラクタ
    State(const State<T> &obj) :x(obj.x), y(obj.y), r(obj.r), vx(obj.vx), vy(obj.vy){}
    //代入演算子
    State<T>& operator=(const State<T> &obj){
        if (this == &obj){
            return *this;
        }
        x = obj.x;
        y = obj.y;
        r = obj.r;
        vx = obj.vx;
        vy = obj.vy;
        return *this;
    }
    //加算演算子
    State<T> operator+(const State<T> &obj)const{
        State<T> tmp;
        tmp.x = this->x + obj.x;
        tmp.y = this->y + obj.y;
        tmp.r = this->r + obj.r;
        tmp.vx = this->vx + obj.vx;
        tmp.vy = this->vy + obj.vy;
        return tmp;
    }
    State<T>& operator+=(const State<T> &obj){
        x += obj.x;
        y += obj.y;
        r += obj.r;
        vx += obj.vx;
        vy += obj.vy;
        return *this;
    }
    //減算演算子
    State<T> operator-(const State<T> &obj)const{
        State<T> tmp;
        tmp.x = x - obj.x;
        tmp.y = y - obj.y;
        tmp.r = r - obj.r;
        tmp.vx = vx - obj.vx;
        tmp.vy = vy - obj.vy;
        return tmp;
    }
    State<T>& operator-=(const State<T> &obj){
        x -= obj.x;
        y -= obj.y;
        r -= obj.r;
        vx -= obj.vx;
        vy -= obj.vy;
        return *this;
    }
    //積演算子
    State<T> operator*(const T &obj) const{
        State<T> tmp;
        tmp.x = x * obj;
        tmp.y = y * obj;
        tmp.r = r * obj;
        tmp.vx = vx * obj;
        tmp.vy = vy * obj;
        return tmp;
    }
    State<T>& operator*=(const T &obj){
        x *= obj;
        y *= obj;
        r *= obj;
        vx *= obj;
        vy *= obj;
        return *this;
    }
    State<T> operator*(const State<T> &obj)const{
        State<T> tmp;
        tmp.x = x*obj.x;
        tmp.y = y*obj.y;
        tmp.r = r*obj.r;
        tmp.vx = vx*obj.vx;
        tmp.vy = vy*obj.vy;
        return tmp;
    }
    //商演算子
    State<T> operator/(const T &obj) const{
        State<T> tmp;
        tmp.x = x / obj;
        tmp.y = y / obj;
        tmp.r = r / obj;
        tmp.vx = vx / obj;
        tmp.vy = vy / obj;
        return tmp;
    }
    State<T>& operator/=(const T &obj){
        x /= obj;
        y /= obj;
        r /= obj;
        vx /= obj;
        vy /= obj;
        return *this;
    }
    bool operator==(const State<T> &obj) {
        if (x == obj.x &&
            y == obj.y &&
            r == obj.r &&
            vx == obj.vx &&
            vy == obj.vy) {
            return true;
        } else {
            return false;
        }
    }
    //値変更
    void set(T x, T y, T r, T vx, T vy){
        this->x = x;
        this->y = y;
        this->r = r;
        this->vx = vx;
        this->vy = vy;
    }

    void clear(){
        x = 0;
        y = 0;
        r = 0;
        vx = 0;
        vy = 0;
    }
    //角度の取得
    double degree() const { return (r * 180.0 / M_PI); }

    // normalize r between -M_PI and M_PI
    void normalize_r() {

        double tmp=State<>::normalize_radian(this->r);
        this->r=tmp;
    }

    static double normalize_radian (double radian) {

        double out=radian;

        if (radian > M_PI) {
            radian -= M_PI * 2.0;
            out=State<>::normalize_radian(radian);
        }
        else if (radian <= -M_PI) {
            radian += M_PI * 2.0;
            out=State<>::normalize_radian(radian);
        }

        return out;
    }

    // Max
    State<T> max(const std::vector<State<T>>& positions){
        State<> max=positions.front();
        for (int i = 1; i < positions.size(); i++){
            if (max.x < positions[i].x){
                max.x = positions[i].x;
            }
            if (max.y < positions[i].y){
                max.y = positions[i].y;
            }
            if (max.r < positions[i].r){
                max.r = positions[i].r;
            }
            if (max.vx < positions[i].v){
                max.vx = positions[i].v;
            }
            if (max.vy < positions[i].v){
                max.vy = positions[i].v;
            }
        }
        return max;
    }
    // Mix
    State<T> min(const std::vector<State<T>>& positions){
        State<> min = positions.front();
        for (int i = 1; i < positions.size(); i++){
            if (min.x > positions[i].x){
                min.x = positions[i].x;
            }
            if (min.y > positions[i].y){
                min.y = positions[i].y;
            }
            if (min.r > positions[i].r){
                min.r = positions[i].r;
            }
            if (min.vx > positions[i].v){
                min.vx = positions[i].v;
            }
            if (min.vy > positions[i].v){
                min.vy = positions[i].v;
            }
        }
        return min;
    }


    //2つのデータの距離の算出
    double operator|(const T &obj) const{
        return std::sqrt(std::pow((static_cast<double>(x)-static_cast<double>(obj.x)), 2) + std::pow((static_cast<double>(y)-static_cast<double>(obj.y)), 2));
    }

    T & operator [](int n)
    {
        switch (n)
        {
            case 0: return this->x;
            case 1: return this->y;
            case 2: return this->r;
            case 3: return this->vx;
            case 4: return this->vy;
            default:
                std::cerr << "No Value! " << std::endl;
                exit;
        }
    }

    /*
    //文字列変換 ※(rad→deg)
    std::string ToString() const{
    std::stringstream ss;
    //ss.setf(std::ios::fixed);
    ss<< x << '\t' << y << '\t' << (r * 180.0 / M_PI);
    return ss.str();
    }
    */
    std::string ToString() const{
        std::stringstream ss;
        ss << x << '\t' << y << '\t' << r << '\t' << vx << '\t' << vy;
        return ss.str();
    }



    /*
    //read
    template<typename T> static void Read(const boost::filesystem::path &filePath, std::vector< State<T> > &obj){
    boost::filesystem::ifstream fin(filePath);
    if(fin.fail()){fin.close(); throw"OpenFile \""+filePath.string()+"\" NotFound";}
    T x, y, th;
    obj.clear();
    fin >> x >> y >> th;
    while(!fin.eof()){
    th *= (M_PI/180.0);
    obj.push_back(std::vector< State<T> >::value_type(x, y, th));
    fin >> x >> y >> th;
    }
    }
    //write
    template<typename T> static void Write(const boost::filesystem::path &filePath, const std::vector< State<T> > &obj, const std::ios_base::openmode &mode = std::ios::out){
    boost::filesystem::ofstream fout(filePath, mode);
    fout.setf(std::ios::fixed);
    for(std::vector< State<T> >::const_iterator itr(obj.begin()); itr != obj.end(); ++itr){
    fout << itr->x << '\t' << itr->y << '\t' << itr->degree() << '\n';
    }
    }
    */
};

// 入力用 演算子オーバーロード
template<typename T> inline std::istream& operator>>(std::istream &is, State<T> &obj){
    char split;
    is >> obj.x >> split >> obj.y >> split >> obj.r >> obj.vx >> obj.vy;
    return is;
}
template<typename T> inline std::istream& operator>>(std::istream &is, std::vector<State<T>> &obj){
    std::string str;
    while (std::getline(is,str))
    {
        std::istringstream istr(str);
        State<T> tmp;
        istr >> tmp;
        obj.push_back(tmp);
    }
    return is;
}

// 出力用 演算子オーバーロード
template<typename T> inline std::ostream& operator<<(std::ostream &os, const State<T> &obj){
    return (os << "x: " << obj.x << ", \t y: " << obj.y << ", \t r: " << obj.r << ", \t vx: " << obj.vx << ", \t vy: " << obj.vy);
}
template<typename T> inline std::ostream& operator<<(std::ostream &os, const std::vector<State<T>> &obj){
    for (const auto &tmp : obj)
    {
        os << tmp << std::endl;
    }
    return os;
}
