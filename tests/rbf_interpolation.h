/*

Copyright (c) 2005-2016, University of Oxford.
All rights reserved.

University of Oxford means the Chancellor, Masters and Scholars of the
University of Oxford, having an administrative office at Wellington
Square, Oxford OX1 2JD, UK.

This file is part of Aboria.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
 * Neither the name of the University of Oxford nor the names of its
   contributors may be used to endorse or promote products derived from this
   software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/


#ifndef RBF_INTERPOLATION_TEST_H_
#define RBF_INTERPOLATION_TEST_H_


#include <cxxtest/TestSuite.h>

#define LOG_LEVEL 1
#include "Aboria.h"
#include <random>

using namespace Aboria;


class OperatorsTest : public CxxTest::TestSuite {
public:

    void test_Eigen(void) {
#ifdef HAVE_EIGEN
        auto funct = [](const double x, const double y) { 
            //return std::exp(-9*std::pow(x-0.5,2) - 9*std::pow(y-0.25,2)); 
            return x; 
        };

        ABORIA_VARIABLE(alpha,double,"alpha value")
        ABORIA_VARIABLE(interpolated,double,"interpolated value")
        ABORIA_VARIABLE(constant2,double,"c2 value")

    	typedef Particles<std::tuple<alpha,constant2,interpolated>,2> ParticlesType;
        typedef position_d<2> position;
       	ParticlesType knots;

       	const double c = 0.1;
        double2 min(0);
        double2 max(1);
        double2 periodic(false);
        
        const int N = 100;
        const int nx = 3;
        const double delta = 1.0/nx;
        ParticlesType::value_type p;

        std::default_random_engine generator;
        std::uniform_real_distribution<double> distribution(0.0,1.0);
        for (int i=0; i<N; ++i) {
            get<position>(p) = double2(distribution(generator),
                                       distribution(generator));
            get<constant2>(p) = std::pow(c,2);  
            knots.push_back(p);
        }


        Symbol<alpha> al;
        Symbol<interpolated> interp;
        Symbol<position> r;
        Symbol<constant2> c2;
        Label<0,ParticlesType> a(knots);
        Label<1,ParticlesType> b(knots);
        One one;
        auto dx = create_dx(a,b);
        Accumulate<std::plus<double> > sum;

        auto G = create_eigen_operator(a,b, 
                    //sqrt(pow(norm(dx),2) + c2[a])
                    exp(-pow(norm(dx),2)/c2[a])
                );
 
        auto P = create_eigen_operator(a,one,
                        1.0
                );
        auto Pt = create_eigen_operator(one,b,
                        1.0
                );
        auto Zero = create_eigen_operator(one,one, 0.);

        auto W = create_block_eigen_operator<2,2>(G, P,
                                                  Pt,Zero);

        Eigen::VectorXd phi(knots.size()+1), gamma(knots.size()+1);
        for (int i=0; i<knots.size(); ++i) {
            const double x = get<position>(knots[i])[0];
            const double y = get<position>(knots[i])[1];
            phi[i] = funct(x,y);
        }
        phi[knots.size()] = 0;

        Eigen::ConjugateGradient<decltype(W), 
            Eigen::Lower|Eigen::Upper,  Eigen::DiagonalPreconditioner<double>> cg;
            //Eigen::Lower|Eigen::Upper,  Eigen::IdentityPreconditioner> cg;
        cg.compute(W);

        gamma = cg.solve(phi);
        std::cout << std::endl << "CG:       #iterations: " << cg.iterations() << ", estimated error: " << cg.error() << std::endl;

        // This could be more intuitive....
        Eigen::Map<Eigen::Matrix<double,N,1>> alpha_wrap(get<alpha>(knots).data());
        alpha_wrap = gamma.segment<N>(0);

        const double beta = gamma(knots.size());

        interp[a] = sum(b,true,al[a]*exp(-pow(norm(r[a]-p),2)/c2[a])) + beta;
        for (int i=0; i<knots.size(); ++i) {
            const double x = get<position>(knots[i])[0];
            const double y = get<position>(knots[i])[1];
            const double truth = funct(x,y);
            const double eval_value = get<interpolated>(knots[i]);
            TS_ASSERT_DELTA(eval_value,truth,2e-3); 
        }

        for (int i=0; i<=nx; ++i) {
            for (int j=0; j<=nx; ++j) {
                double2 p(i*delta,j*delta);
                const double truth  = funct(i*delta,j*delta);
                //const double eval_value = eval(sum(a,true,al*sqrt(pow(norm(r[a]-p),2) + c2[a]))) + beta;
                const double eval_value = eval(sum(a,true,al[a]*exp(-pow(norm(r[a]-p),2)/c2[a]))) + beta;
                TS_ASSERT_DELTA(eval_value,truth,2e-3); 
            }
        }

        
#endif // HAVE_EIGEN
    }


};

#endif /* RBF_INTERPOLATION_TEST_H_ */
