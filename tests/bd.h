/*
 * bd_symbolic.cpp
 *
 *  Created on: 30 Jan 2014
 *      Author: mrobins
 */
#ifndef BD_TEST_H_
#define BD_TEST_H_

#include <cxxtest/TestSuite.h>

//[bd
#include <random>
typedef std::mt19937 generator_type;
generator_type generator;
#include "Aboria.h"
using namespace Aboria;

//<-
class BDTest : public CxxTest::TestSuite {
public:

    void test_bd(void) {
//->
//=int main() {
        ABORIA_VARIABLE(radius,double,"radius")

        typedef Particles<std::tuple<radius>> spheres_type;
        typedef Particles<> points_type;
        typedef position_d<3> position;
        spheres_type spheres;

        const double L = 10.0;
        const double D = 1.0;
        const double dt = 0.1;
        const double timesteps = 1000;

        spheres.push_back(double3(0,0,0));
        get<radius>(spheres[0]) = 1.0;
        spheres.push_back(double3(5,0,0));
        get<radius>(spheres[1]) = 2.0;
        spheres.push_back(double3(0,-5,0));
        get<radius>(spheres[2]) = 1.5;
        spheres.push_back(double3(0,0,5));
        get<radius>(spheres[3]) = 1.0;

        points_type points;
        std::uniform_real_distribution<double> uni(-L/5,L/5);
        for (int i = 0; i < 1000; ++i) {
            points.push_back(double3(uni(generator),uni(generator),uni(generator)));
        }


        points.init_neighbour_search(double3(-L/5,-L/5,-L/5),double3(L/5,L/5,L/5),4,bool3(true,true,true));
        spheres.init_neighbour_search(double3(-L,-L,-L),double3(L,L,L),4,bool3(false,false,false));

        Symbol<position> p;
        Symbol<radius> r;
        Symbol<alive> alive_;
        Label<0,spheres_type > a(spheres);
        Label<1,spheres_type > b(spheres);
        Label<0,points_type > i(points);
        Label<1,points_type > j(points);
        auto dx = create_dx(i,b);
        Normal N;
        VectorSymbolic<double> vector;      
        Accumulate<std::bit_or<bool> > any;
        Accumulate<std::plus<double3> > sum;

        int count_before=0;
        for(auto point: points) {
            if ((get<position>(point)-get<position>(spheres[0])).norm() < get<radius>(spheres[0])) {
                count_before++;
            }
        }

        /*
         * Kill any points within spheres
         */
        alive_[i] = !any(b, norm(dx) < r[b],true);


        int count_after=0;
        for(auto point: points) {
            if ((get<position>(point)-get<position>(spheres[0])).norm() < get<radius>(spheres[0])) {
                count_after++;
            } 
        }

        std::cout <<" found "<<count_before<<" before and "<<count_after<<" after"<<std::endl;

        /*
         * Diffusion step for points and reflect off spheres
         */
        for (int ts = 1; ts < timesteps; ++ts) {
            if (ts%10==0) {
                std::cout << "." << std::flush;
            }
            p[i] += std::sqrt(2*D*dt)*vector(N,N,N);
            p[i] += sum(b, norm(dx) < r[b],
                        -2*(r[b]/norm(dx)-1)*dx );

        }
        std::cout << std::endl;
    }
//]

};

#endif /* BD_TEST_H_ */

