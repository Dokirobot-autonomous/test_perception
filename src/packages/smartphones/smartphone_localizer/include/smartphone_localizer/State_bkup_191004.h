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
	T v;
	const int par_num = 4;	//	パラメータの数(x,y,θ,v)

	//コンストラクタ
	State() :x(0), y(0), r(0), v(0){}
	State(T x, T y, T r, T v) :x(x), y(y), r(r), v(v){}
	//コピーコンストラクタ
	State(const State<T> &obj) :x(obj.x), y(obj.y), r(obj.r), v(obj.v){}
	//代入演算子
	State<T>& operator=(const State<T> &obj){
		if (this == &obj){
			return *this;
		}
		x = obj.x;
		y = obj.y;
		r = obj.r;
		v = obj.v;
		return *this;
	}
	//加算演算子
	State<T> operator+(const State<T> &obj)const{
		State<T> tmp;
		tmp.x = this->x + obj.x;
		tmp.y = this->y + obj.y;
		tmp.r = this->r + obj.r;
		tmp.v = this->v + obj.v;
		return tmp;
	}
	State<T>& operator+=(const State<T> &obj){
		x += obj.x;
		y += obj.y;
		r += obj.r;
		v += obj.v;
		return *this;
	}
	//減算演算子
	State<T> operator-(const State<T> &obj)const{
		State<T> tmp;
		tmp.x = x - obj.x;
		tmp.y = y - obj.y;
		tmp.r = r - obj.r;
		tmp.v = v - obj.v;
		return tmp;
	}
	State<T>& operator-=(const State<T> &obj){
		x -= obj.x;
		y -= obj.y;
		r -= obj.r;
		v -= obj.v;
		return *this;
	}
	//積演算子
	State<T> operator*(const T &obj) const{
		State<T> tmp;
		tmp.x = x * obj;
		tmp.y = y * obj;
		tmp.r = r * obj;
		tmp.v = v * obj;
		return tmp;
	}
	State<T>& operator*=(const T &obj){
		x *= obj;
		y *= obj;
		r *= obj;
		v *= obj;
		return *this;
	}
	State<T> operator*(const State<T> &obj)const{
		State<T> tmp;
		tmp.x = x*obj.x;
		tmp.y = y*obj.y;
		tmp.r = r*obj.r;
		tmp.v = v*obj.v;
		return tmp;
	}
	//商演算子
	State<T> operator/(const T &obj) const{
		State<T> tmp;
		tmp.x = x / obj;
		tmp.y = y / obj;
		tmp.r = r / obj;
		tmp.v = v / obj;
		return tmp;
	}
	State<T>& operator/=(const T &obj){
		x /= obj;
		y /= obj;
		r /= obj;
		v /= obj;
		return *this;
	}
	//値変更
	void set(T x, T y, T r, T v){
		this->x = x;
		this->y = y;
		this->r = r;
		this->v = v;
	}

	void clear(){
		x = 0;
		y = 0;
		r = 0;
		v = 0;
	}
	//角度の取得
	double degree()const{ return (r * 180.0 / M_PI); }

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
			if (max.v < positions[i].v){
				max.v = positions[i].v;
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
			if (min.v > positions[i].v){
				min.v = positions[i].v;
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
		case 3: return this->v;
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
		ss << x << '\t' << y << '\t' << r << '\t' << v;
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
	is >> obj.x >> split >> obj.y >> split >> obj.r >> obj.v;
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
	return (os << "x: " << obj.x << ", \t y: " << obj.y << ", \t r: " << obj.r << ", \t v: " << obj.v);
}
template<typename T> inline std::ostream& operator<<(std::ostream &os, const std::vector<State<T>> &obj){
	for (const auto &tmp : obj)
	{
		os << tmp << std::endl;
	}
	return os;
}
