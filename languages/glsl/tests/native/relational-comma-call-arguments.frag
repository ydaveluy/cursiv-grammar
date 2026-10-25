#version 460
bool f(bool a,bool b){return a&&b;}void main(){float a=1.0,b=2.0,c=3.0,d=4.0;bool x=f(a<b,c>(d));}