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
#include <exception>
#include <cstdlib>
#include <execinfo.h>

#include <boost/exception/info.hpp>
#include <libsgp4/Globals.h>
#include <SML/sml.hpp>
#include <SML/constants.hpp>
#include <SML/basicFunctions.hpp>

#include "CppProject/TleGen.hpp"
#include "CppProject/randomGen.hpp"


typedef double Real;
typedef std::vector< Real > Vector;
typedef std::vector< Real > Vector6;
typedef std::vector< Real > Vector3;
typedef std::vector< Real > Vector2;
typedef std::vector < std::vector < Real > > Vector2D;

int main(void)
{
    // some constants values are defined here
    const double km2m = 1000; // conversion from km to m
    // earth radius
    const double EarthRadius = kXKMPER * km2m; // unit m
    const double EarthDiam = 2 * EarthRadius;
    // grav. parameter 'mu' of earth
    // const double muEarth = kMU*( pow( 10, 9 ) ); // unit m^3/s^2
    const int bypass = true; // keep this true, this just enables executing an alternate code snippet that takes care of thrown exceptions from ATOM

    if(bypass == false){
        // initialize input parameters for the function generating random orbital elements. Description can be
        // found in randomKepElem.hpp for each of the following parameters. 
        const Vector2 range_a      = { (EarthDiam+100000), (EarthDiam+1000000) }; 
        const Vector2 range_e      = { 0, 1 };
        const Vector2 range_i      = { 0, sml::convertDegreesToRadians( 180.0 ) };
        const Vector2 range_raan   = { 0, sml::convertDegreesToRadians( 360.0 ) };
        const Vector2 range_w      = { 0, sml::convertDegreesToRadians( 360.0 ) };
        const Vector2 range_E      = { 0, sml::convertDegreesToRadians( 360.0 ) };
        const int limit            = 100;
        Vector2D randKep( limit, std::vector< Real >( 6 ) );
        Vector semiAxis( limit ); // storage for random semi major axis values
        Vector eccentricity( limit ); // storage for random eccentricity values
        Vector inclination( limit ); // storage for random inclination values
        Vector RAAN( limit ); // storage for random RAAN values
        Vector AOP( limit ); // storage for random AOP values
        Vector EA( limit ); // storage for random EA values

        // call the function to generate random keplerian orbital elements. Values are stored in randKep in a 2D
        // vector format. A single row represents one set of orbital elements, arranged in the same order as the
        // input argument order of the elements. 
        randomGen::randomGen( range_a, limit, semiAxis );
        randomGen::randomGen( range_e, limit, eccentricity );
        randomGen::randomGen( range_i, limit, inclination );
        randomGen::randomGen( range_raan, limit, RAAN );
        randomGen::randomGen( range_w, limit, AOP );
        randomGen::randomGen( range_E, limit, EA );
        for(int i = 0; i < limit; i++)
        {
            randKep[ i ][ 0 ] = semiAxis[ i ];
            randKep[ i ][ 1 ] = eccentricity[ i ];
            randKep[ i ][ 2 ] = inclination[ i ];
            randKep[ i ][ 3 ] = RAAN[ i ];
            randKep[ i ][ 4 ] = AOP[ i ];
            randKep[ i ][ 5 ] = EA[ i ];
        }

        
        // remove orbital elements where the radius of perigee is inside earth
        Vector2D randKepElem( limit, std::vector< Real >( 6 ) );
        int newLimit = limit; // whenever a row of cartesian (keplerian) elements is removed, the limit will have to be changed. 
        Real radiusPerigee = 0; // variable storing the radius of Perigee for an orbit. this is used in our checking condition
        std::vector< int > rowsToDelete; // stores the row numbers which have to be deleted
        int insideCounter = 0; // to count the number of times radius of perigee is inside the Earth. 
        int outsideCounter = 0; // to count the number of times the radius of perigee is beyond a cetain upper limit
        int newIndex = 0; // index counter for the second 2D vector
        for(int j = 0; j < newLimit; j++)
        {
            radiusPerigee = randKep[ j ][ 0 ] * (1 - randKep[ j ][ 1 ]);
            if(radiusPerigee <= EarthRadius )
            {
                insideCounter = insideCounter + 1; // counter increments everytime the condition is true
            }
            if(radiusPerigee >= (EarthRadius+2000000))
            {
                outsideCounter = outsideCounter + 1;
            }
            if(radiusPerigee > EarthRadius && radiusPerigee < (EarthRadius+2000000)) // this is obv not the most efficient way
            {
                randKepElem[ newIndex ] = randKep[ j ];
                newIndex++;
            }
        } 
        randKepElem.erase( randKepElem.begin() + newIndex, randKepElem.end() ); // delete the left over rows in the final random keplerian element vector
        std::vector< std::vector < Real > > ().swap(randKep); // create an empty vector with no memory allocated to it ...
        // ... and swap it with the vector which you want to delete and deallocate the memory
        std::cout << "Inside counter value = " << insideCounter << std::endl;
        std::cout << "Outside counter value = " << outsideCounter << std::endl;
        std::cout << "Usefull sets left = " << limit - (insideCounter + outsideCounter) << std::endl;
        newLimit = randKepElem.size();
        std::cout << "Number of final rows in randKepElem 2D vector = " << newLimit << std::endl;

        // file storage
        std::ofstream tlefile;
        tlefile.open("TLEfile.csv", std::ofstream::app);
        tlefile << "Solver Status Summary" << "," << "Iteration Count" << "," << "semi-major axis [km]" << "," << "eccentricity" << ",";
        tlefile << "Inclination [deg]" << "," << "RAAN [deg]" << "," << "AOP [deg]" << "," << "Eccentric Anomaly [deg]" << std::endl;
        // Generate the TLEs for the final random set of orbital elements
        std::string SolverStatus;
        int IterationCount;
        // some other bookkeeping variables, these are not used in the convert to tle function
        std::size_t findSuccess;
        for(int i = 0; i < newLimit; i++)
        {
            IterationCount = 0;
            TleGen::TleGen( randKepElem[ i ], SolverStatus, IterationCount ); // giving one set of orbital elements one by one to the tle generator
            findSuccess = SolverStatus.find("success");    
            if(findSuccess == std::string::npos){
                std::cout << "TLE Conversion Failed" << std::endl;
                tlefile << "Failed" << "," << IterationCount << ",";
                tlefile << ( randKepElem[ i ][ 0 ]/1000 ) << ",";
                tlefile << randKepElem[ i ][ 1 ] << ",";
                tlefile << sml::convertRadiansToDegrees( randKepElem[ i ][ 2 ] ) << ",";
                tlefile << sml::convertRadiansToDegrees( randKepElem[ i ][ 3 ] ) << ",";
                tlefile << sml::convertRadiansToDegrees( randKepElem[ i ][ 4 ] ) << ",";
                tlefile << sml::convertRadiansToDegrees( randKepElem[ i ][ 5 ] ) << std::endl;
            }
            else{
                std::cout << "TLE conversion success" << std::endl;
                tlefile << "Success" << "," << IterationCount << ",";
                tlefile << ( randKepElem[ i ][ 0 ]/1000 ) << ",";
                tlefile << randKepElem[ i ][ 1 ] << ",";
                tlefile << sml::convertRadiansToDegrees( randKepElem[ i ][ 2 ] ) << ",";
                tlefile << sml::convertRadiansToDegrees( randKepElem[ i ][ 3 ] ) << ",";
                tlefile << sml::convertRadiansToDegrees( randKepElem[ i ][ 4 ] ) << ",";
                tlefile << sml::convertRadiansToDegrees( randKepElem[ i ][ 5 ] ) << std::endl;
            }
            
        }
        tlefile << std::endl << std::endl;
        tlefile.close();
    }
    else{
            const int newLimit = 1000; // number of random elementss to be generated
            Vector2D randKepElem( newLimit, std::vector < Real >( 6 ) ); // general 2D vector to store the randomly generated keplerian elements in one place
            int indexer = 0; // used to keep track of which orbit or set of random orbital elements is being run in the simulation
            Vector RadiusOfPerigee( newLimit ); // to store radius of perigee value for each random orbit generated
            
            std::ofstream tlefile; // output file handle
            tlefile.open( "TLEfile3.csv", std::ofstream::app ); // open the file and append the data
            tlefile << "Solver Status Summary" << "," << "Iteration Count" << "," << "semi-major axis [km]" << "," << "SMA Seed" << ",";
            tlefile << "eccentricity" << "," << "ECC Seed" << ",";
            tlefile << "Inclination [deg]" << "," << "INC Seed" << ",";
            tlefile << "RAAN [deg]" << "," << "RAAN Seed" << ",";
            tlefile << "AOP [deg]" << "," << "AOP Seed" << ","; 
            tlefile << "Eccentric Anomaly [deg]" << "," << "EA Seed" << "," << "Radius of Perigee [km]" << std::endl;
            
            // bookkeeping variables, these are not used in the convert to tle function
            std::size_t findSuccess;

            Vector2 range_a = { (EarthDiam + 10000 * km2m), (EarthDiam + 90000 * km2m) }; // range for semi major axis
            Vector2 range_e = { 0.0, 1.0 }; // range for eccentricity
            Vector2 range_i = { 0.0, 180.0 }; // range for inclination
            Vector2 range_raan = { 0.0, 360.0 }; // range for raan
            Vector2 range_w = { 0.0, 360.0 }; // range for argument of perigee
            Vector2 range_EA = { 0.0, 360.0 }; // range for eccentric anomaly

            Vector eccentricity( newLimit ); // init. vector to store eccentricity values
            Vector semiAxis( newLimit ); // initialize vector of size newLimit 
            Vector inclination( newLimit ); // vector to store random inclination values
            Vector raan( newLimit ); // vector to store random raan values
            Vector aop( newLimit ); // vector to store random aop values
            Vector EA( newLimit ); // vector to store random EA values

            // seed values for each orbital element, to be used in the pseudo random element generator algorithm. These seed values can be used to 
            // regenerate the psuedo random sequence at any given time in future. 
            const int semiSeed = 400; 
            const int eccSeed = 400;
            const int incSeed = 400; 
            const int raanSeed = 300;
            const int aopSeed = 200;
            const int eaSeed = 100;

            // random value generation
            randomGen::randomGenWithSeed( range_a, newLimit, semiAxis, semiSeed ); 
            randomGen::randomGenWithSeed( range_e, newLimit, eccentricity, eccSeed );
            randomGen::randomGenWithSeed( range_i, newLimit, inclination, incSeed );
            randomGen::randomGenWithSeed( range_raan, newLimit, raan, raanSeed );
            randomGen::randomGenWithSeed( range_w, newLimit, aop, aopSeed );
            randomGen::randomGenWithSeed( range_EA, newLimit, EA, eaSeed );    

            for(int i = 0; i < newLimit; i++)
            {
                randKepElem[ i ][ 0 ] = semiAxis[ i ]; // semi major axis
                randKepElem[ i ][ 1 ] = eccentricity[ i ]; // eccentricity
                randKepElem[ i ][ 2 ] = sml::convertDegreesToRadians( inclination[ i ] ); // inclination
                randKepElem[ i ][ 3 ] = sml::convertDegreesToRadians( raan[ i ] ); // RAAN
                randKepElem[ i ][ 4 ] = sml::convertDegreesToRadians( aop[ i ] ); // AOP
                randKepElem[ i ][ 5 ] = sml::convertDegreesToRadians( EA[ i ] ); // EA
                RadiusOfPerigee[ i ] = semiAxis[ i ] * ( 1 - eccentricity[ i ] );
            }
            while( indexer < newLimit)
            {
                std::string SolverStatus;
                int IterationCount;
                try
                {
                    indexer++;
                    // std::cout << "indexer = " << indexer << std::endl;
                    // std::cout << "Radius of Perigee = " << RadiusOfPerigee[ indexer - 1 ]/1000 << std::endl;
                    // std::cout << "Semi Axis = " << randKepElem[ indexer - 1 ][ 0 ]/1000 << std::endl;
                    // std::cout << "Eccentricity = " << randKepElem[ indexer - 1 ][ 1 ] << std::endl;
                    // std::cout << "Inclination = " << sml::convertRadiansToDegrees( randKepElem[ indexer - 1 ][ 2 ] ) << std::endl;

                    TleGen::TleGen( randKepElem[ indexer - 1 ], SolverStatus, IterationCount );
                    // std::cout << SolverStatus << std::endl;
                    findSuccess = SolverStatus.find("success");    
                    if(findSuccess == std::string::npos){
                            // std::cout << "TLE Conversion Failed" << std::endl;
                            tlefile << "Failed" << "," << IterationCount << ",";
                            tlefile << ( randKepElem[ indexer - 1 ][ 0 ]/1000 ) << "," << semiSeed << ",";
                            tlefile << randKepElem[ indexer - 1 ][ 1 ] << "," << eccSeed << ",";
                            tlefile << sml::convertRadiansToDegrees( randKepElem[ indexer - 1 ][ 2 ] ) << "," << incSeed << ",";
                            tlefile << sml::convertRadiansToDegrees( randKepElem[ indexer - 1 ][ 3 ] ) << "," << raanSeed << ",";
                            tlefile << sml::convertRadiansToDegrees( randKepElem[ indexer - 1 ][ 4 ] ) << "," << aopSeed << ",";
                            tlefile << sml::convertRadiansToDegrees( randKepElem[ indexer - 1 ][ 5 ] ) << "," << eaSeed << ",";
                            tlefile << RadiusOfPerigee[ indexer - 1 ]/1000 << std::endl;   
                    }
                    else{
                            // std::cout << "TLE conversion success" << std::endl;
                            tlefile << "Success" << "," << IterationCount << ",";
                            tlefile << ( randKepElem[ indexer - 1 ][ 0 ]/1000 ) << "," << semiSeed << ",";
                            tlefile << randKepElem[ indexer - 1 ][ 1 ] << "," << eccSeed << ",";
                            tlefile << sml::convertRadiansToDegrees( randKepElem[ indexer - 1 ][ 2 ] ) << "," << incSeed << ",";
                            tlefile << sml::convertRadiansToDegrees( randKepElem[ indexer - 1 ][ 3 ] ) << "," << raanSeed << ",";
                            tlefile << sml::convertRadiansToDegrees( randKepElem[ indexer - 1 ][ 4 ] ) << "," << aopSeed << ",";
                            tlefile << sml::convertRadiansToDegrees( randKepElem[ indexer - 1 ][ 5 ] ) << "," << eaSeed << ",";
                            tlefile << RadiusOfPerigee[ indexer - 1 ]/1000 << std::endl;   
                    }
                }
                catch(const std::exception& err)
                {
                    // std::cout << "Error Caught = ";
                    // std::cout << err.what() << std::endl;
                    tlefile << "Exception Caught = " << err.what() << "," << ",";
                    tlefile << semiAxis[ indexer - 1 ]/1000 << "," << semiSeed << ",";
                    tlefile << eccentricity[ indexer - 1 ] << "," << eccSeed << ",";
                    tlefile << inclination[ indexer - 1 ] << "," << incSeed << ",";
                    tlefile << raan[ indexer - 1 ] << "," << raanSeed << ",";
                    tlefile << aop[ indexer - 1 ] << "," << aopSeed << ",";
                    tlefile << EA[ indexer - 1 ] << "," << eaSeed << ",";
                    tlefile << RadiusOfPerigee[ indexer - 1 ]/1000 << std::endl;   
                }
            }
            tlefile << std::endl << std::endl;
            tlefile.close();
        }
   return EXIT_SUCCESS;
}

    


