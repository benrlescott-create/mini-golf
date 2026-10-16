#pragma once
// Engine-independent metre-space simulation, shared by gameplay and checks.
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace Putt
{
constexpr double Radius=.19;
struct Vec { double x=0,z=0; };
struct Terrace { double z=0,h=0; };
struct Obstacle { bool circle=false;double x=0,z=0,r=0,w=0,d=0,angle=0,spin=0; };
struct Course
{
    std::string name,tip;int par=2;bool boost=false;
    Vec start,cup;std::vector<Vec> polygon;
    std::vector<Terrace> elevation;std::vector<Obstacle> obstacles;
};
struct Ball
{
    double x=0,z=0,y=0,vx=0,vz=0;bool sunk=false,boosted=false;
    double speed()const{return std::hypot(vx,vz);}
};
inline double height(const Course& c,double z)
{
    const auto& p=c.elevation;if(p.empty())return 0;
    if(z<=p.front().z)return p.front().h;
    for(size_t i=1;i<p.size();++i)if(z<=p[i].z)
    {double t=(z-p[i-1].z)/(p[i].z-p[i-1].z);return p[i-1].h+(p[i].h-p[i-1].h)*t*t*(3-2*t);}
    return p.back().h;
}
inline double slope(const Course& c,double z)
{
    const auto& p=c.elevation;if(p.empty()||z<=p.front().z||z>=p.back().z)return 0;
    for(size_t i=1;i<p.size();++i)if(z<=p[i].z)
    {double t=(z-p[i-1].z)/(p[i].z-p[i-1].z);return (p[i].h-p[i-1].h)*6*t*(1-t)/(p[i].z-p[i-1].z);}
    return 0;
}
inline double ballY(const Course& c,double z){double s=slope(c,z);return height(c,z)+Radius*std::sqrt(1+s*s)+.025;}
inline Ball create(const Course& c){Ball b;b.x=c.start.x;b.z=c.start.z;b.y=ballY(c,b.z);return b;}
inline bool strike(Ball& b,Vec direction,double power)
{
    double len=std::hypot(direction.x,direction.z);
    if(b.sunk||b.speed()>.01||len<.001||power<.025)return false;
    double scale=std::clamp(power,0.0,1.0)*15/len;
    b.vx=direction.x*scale;b.vz=direction.z*scale;b.boosted=false;return true;
}
inline double resolve(Ball& b,double nx,double nz,double penetration,double bounce)
{
    b.x+=nx*penetration;b.z+=nz*penetration;double dot=b.vx*nx+b.vz*nz;
    if(dot>=0)return 0;b.vx-=(1+bounce)*dot*nx;b.vz-=(1+bounce)*dot*nz;return -dot;
}
inline double step(Ball& b,const Course& c,double dt,double time)
{
    if(b.sunk)return 0;
    double s=slope(c,b.z);b.vz-=9.81*(5.0/7)*s/(1+s*s)*dt;
    b.x+=b.vx*dt;b.z+=b.vz*dt;double hit=0;
    for(size_t i=0;i<c.polygon.size();++i)
    {
        auto a=c.polygon[i],d=c.polygon[(i+1)%c.polygon.size()];
        double dx=d.x-a.x,dz=d.z-a.z;
        double t=std::clamp(((b.x-a.x)*dx+(b.z-a.z)*dz)/(dx*dx+dz*dz),0.0,1.0);
        double nx=b.x-a.x-dx*t,nz=b.z-a.z-dz*t,len=std::hypot(nx,nz);
        if(len<Radius&&len>.00001)hit=std::max(hit,resolve(b,nx/len,nz/len,Radius-len,.78));
    }
    for(auto o:c.obstacles)
    {
        if(o.circle)
        {
            double dx=b.x-o.x,dz=b.z-o.z,len=std::hypot(dx,dz),r=Radius+o.r;
            if(len<r&&len>.00001)hit=std::max(hit,resolve(b,dx/len,dz/len,r-len,1.05));
        }
        else
        {
            double angle=o.angle+o.spin*time,co=std::cos(angle),si=std::sin(angle),dx=b.x-o.x,dz=b.z-o.z;
            double lx=co*dx+si*dz,lz=-si*dx+co*dz;
            double nx=lx-std::clamp(lx,-o.w,o.w),nz=lz-std::clamp(lz,-o.d,o.d),len=std::hypot(nx,nz),pen=Radius-len;
            if(len>=Radius)continue;
            if(len<.00001)
            {
                if(o.w-std::abs(lx)<o.d-std::abs(lz)){nx=lx>=0?1:-1;nz=0;pen=Radius+o.w-std::abs(lx);}
                else{nx=0;nz=lz>=0?1:-1;pen=Radius+o.d-std::abs(lz);}len=1;
            }
            hit=std::max(hit,resolve(b,(co*nx-si*nz)/len,(si*nx+co*nz)/len,pen,.8));
        }
    }
    double speed=b.speed(),dist=std::hypot(b.x-c.cup.x,b.z-c.cup.z);b.y=ballY(c,b.z);
    if(dist<.34&&speed<5.2){b.sunk=true;b.vx=b.vz=0;return hit;}
    if(dist<.65&&speed<2){b.vx+=(c.cup.x-b.x)*dt*4;b.vz+=(c.cup.z-b.z)*dt*4;}
    if(c.boost&&!b.boosted&&b.z>3.4&&b.z<4.1&&std::abs(b.x)<2&&speed>.1)
    {b.vx*=1.5;b.vz*=1.5;b.boosted=true;}
    double friction=std::exp(-.72*dt);b.vx*=friction;b.vz*=friction;
    if(b.speed()<.13&&(std::abs(slope(c,b.z))<.035||hit>0))b.vx=b.vz=0;
    return hit;
}
}
