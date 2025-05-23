#pragma once
#include "Mesh.h"

inline Mesh makeSphere(int lat=20,int lon=40,float radius=1.f){
    std::vector<Vertex> v;
    std::vector<unsigned> idx;
    for(int y=0;y<=lat;++y){
        float t=float(y)/lat,  phi=t*glm::pi<float>();
        for(int x=0;x<=lon;++x){
            float s=float(x)/lon, theta=s*glm::two_pi<float>();
            glm::vec3 p = {sin(phi)*cos(theta), cos(phi), sin(phi)*sin(theta)};
            v.push_back({p*radius, p, {s,t}});
        }
    }
    for(int y=0;y<lat;++y){
        for(int x=0;x<lon;++x){
            int cur = y*(lon+1)+x, nxt=cur+lon+1;
            idx.insert(idx.end(), {
                static_cast<unsigned int>(cur),
                static_cast<unsigned int>(nxt),
                static_cast<unsigned int>(cur + 1),
                static_cast<unsigned int>(cur + 1),
                static_cast<unsigned int>(nxt),
                static_cast<unsigned int>(nxt + 1)
            });
        }
    }
    return Mesh(std::move(v), std::move(idx));
}

