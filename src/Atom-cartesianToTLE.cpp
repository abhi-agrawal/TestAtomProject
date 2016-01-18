/*
 * Copyright (c) 2016 Abhishek Agrawal (abhishek.agrawal@protonmail.com)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

// This code will convert multiple cartesian state vectors 
// into TLE and check where exactly the Atom is failing. The cartesian
// vectors are generated by converting randomly generated keplerian elements. 
// The conversion is achieved through the pykep library of ESA.  

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <fstream>

#include <gsl/gsl_multiroots.h>
#include <gsl/gsl_vector.h>
 	
#include <libsgp4/DateTime.h>
#include <libsgp4/Eci.h>
#include <libsgp4/Globals.h>
#include <libsgp4/SGP4.h>
#include <libsgp4/Tle.h>

#include <Astro/astro.hpp>
#include <SML/sml.hpp>

#include <Atom/printFunctions.hpp>

#include <Astro/orbitalElementConversions.hpp>
#include <Atom/convertCartesianStateToTwoLineElements.hpp>

#include "CppProject/randomKepElem.hpp"

typedef double Real;
typedef std::vector< Real > Vector6;
typedef std::vector< Real > Vector3;
typedef std::vector< Real > Vector2;
typedef std::vector < std::vector < Real > > Vector2D;

int main(void)
{
    // some constants values are defined here
    // earth radius
    // grav. parameter 'mu' of earth
    // value of Pi

	// initialize input parameters for the function generating random orbital elements. Description can be
    // found in randomKepElem.hpp for each of the following parameters. 
    const Vector2 range_a      = { 0, 10 };
    const Vector2 range_e      = { 20, 30 };
    const Vector2 range_i      = { 40, 50 };
    const Vector2 range_raan   = { 60, 70 };
    const Vector2 range_w      = { 80, 90 };
    const Vector2 range_E      = { 100, 110 };
    const int limit            = 100;
    Vector2D randKepElem( limit, std::vector< Real >( 6 ) );

    // call the function to generate random keplerian orbital elements. Values are stored in randKepElem in a 2D
    // vector format. A single row represents one set of orbital elements, arranged in the same order as the
    // input argument order of the elements. 
    randomKepElem::randomKepElem( range_a, range_e, range_i, range_raan, range_w, range_E, limit, randKepElem );

    // test output to check if the random numbers are generated without any errors
    std::cout << randKepElem[ 0 ][ 1 ] << std::endl;

    //store randomly generated keplerian element sets into a CSV file for easy viewing and use in future debugging of ATOM
    std::ofstream RandomKepElemFile;
    RandomKepElemFile.open("RandomKepElemFile.csv"); //file will be overwritten each time the code is run unless the name is changed here and the code recompiled
    RandomKepElemFile << "semi-major axis [km]" << "," << "eccentricity" << ",";
    RandomKepElemFile << "Inclination [deg]" << "," << "RAAN [deg]" << ",";
    RandomKepElemFile << "AOP [deg]" << "," << "Eccentric Anomaly [deg]" << std::endl;
    for(int i = 0; i < limit; i++)
    {
        for(int j = 0; j < 6; j++)
        {
            RandomKepElemFile << randKepElem[ i ][ j ] << ",";
        }
        RandomKepElemFile << std::endl;
    }
    RandomKepElemFile.close();

    // generate sets of cartesian elements corresponding to each of the psuedo random orbital element set. 
    // The conversion from keplerian elements to the cartesian elements is done using the PyKep library from ESA.
    

	// Tle convertedTle = atom::convertCartesianStateToTwoLineElements< Real, Vector6 >( cartesianState, DateTime( ) );

	// std::cout << convertedTle << std::endl; 

    return EXIT_SUCCESS;
}

