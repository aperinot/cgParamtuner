#include <iostream>
#include <algorithm>
#include <cstring>
#include <string>
#include <math.h>
#include <array>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <map>

const int MAX_SCORES = 5;

float SCORE_BASES[MAX_SCORES] {
    10000, 12, 42, 5, 0.5
};

float SCORE_RANGES[MAX_SCORES] {
   1000, 10, 1, 2, 0.5
};

float MIN_SCORE_FOR_UPGRADE = 55.0;
const int GAMES_POP = 100;
const int GAMES_VAL = 200;

std::string CSB_BINARY("/path/to/csb_binary/csb");
std::string BRUTAL_DIR("/path/to/brutal_tester/");

std::string BRUTAL_BINARY("java -jar " + BRUTAL_DIR + "/cg-brutaltester-0.0.1-SNAPSHOT.jar -t 4 -s -r " + BRUTAL_DIR + "/csbref");



double randomReal(){
    return double(rand()) / double(RAND_MAX);
}

double randomGene(uint8_t scoreId) {
    double sign = randomReal() > 0.5 ? 1.0 : -1.0;
    double gene = SCORE_BASES[scoreId] + SCORE_RANGES[scoreId] * sign * randomReal();
    return gene <= 0 ? 0 : gene;
}


class Individual {
public:
    uint8_t entries;
    float fitness;
    float genes[MAX_SCORES];

    Individual(const Individual& other) : entries(other.entries), fitness(other.fitness) {
        memcpy(genes, other.genes, sizeof(genes));
    }

    Individual() : entries(0), fitness(0) {}
    Individual(uint8_t entries) : entries(entries), fitness(0) {
        for (uint8_t i = 0; i < entries; ++i) {
            genes[i] = randomGene(i);
        }
    }

    Individual(uint8_t entries, float (&data)[MAX_SCORES], float fitness) : entries(entries), fitness(fitness) {
        memcpy(genes, data, sizeof(data));
    }

    void mutate(double mutationRate) {
      for (int i = 0; i < entries; ++i) {
          if (randomReal() <= mutationRate) {
             genes[i] = randomGene(i);
          }
      }
    }

    void getValues(std::ostream& ostr) {
        for (int i = 0; i < entries; ++i) {
            ostr << " " << genes[i];
        }
    }

    int hash() {
        int h = std::hash<float>{}(genes[0]);

        for (int i = 1; i < entries; ++i) {
            int h2 = std::hash<float>{}(genes[i]);
            h = h ^ (h2 << 1);
        }

        return h;
    }

};


class Population {
public:
    uint16_t populationSize;
    std::vector<Individual> individuals;
    static constexpr double uniformRate = 0.5;
    static constexpr double mutationRate = 0.25;
    static constexpr int tournamentSize = 4;

    Population(const Population& other) : populationSize(other.populationSize), individuals(other.individuals) { }
    Population(uint16_t populationSize) : populationSize(populationSize) {
        individuals.reserve(populationSize);
    }

    void init(float (&data)[MAX_SCORES]) {
        individuals.clear();
        individuals.emplace_back(Individual(MAX_SCORES, data, 50.0));

        for (int i = 1; i < populationSize; i++) {
            individuals.push_back(individuals[0]);
            individuals.back().mutate(mutationRate);
        }
    }


    Individual getFittest() {
        Individual fittest = individuals[0];
        for (uint16_t i = 1; i < populationSize; i++) {
            if (fittest.fitness < individuals[i].fitness) {
                fittest = individuals[i];
            }
        }
        return fittest;
    }


    Population evolve() {
        Population newPopulation(populationSize);

        newPopulation.individuals.push_back(getFittest());

        for (uint16_t i = 1; i < populationSize; ++i) {
            Individual indiv1 = tournamentSelection();
            Individual indiv2 = tournamentSelection();
            newPopulation.individuals.emplace_back(crossover(indiv1, indiv2));
            newPopulation.individuals.back().mutate(mutationRate);
        }

        return newPopulation;
    }


    Individual&& crossover(Individual& indiv1, Individual& indiv2) {
        Individual newSol(indiv1.entries);

        for (uint8_t i = 0; i < newSol.entries; i++) {
            if (randomReal() <= uniformRate) {
                newSol.genes[i] = indiv1.genes[i];
            } else {
                newSol.genes[i] = indiv2.genes[i];
            }
        }
        return std::move(newSol);
    }

    Individual tournamentSelection() {
        static Population tournament(tournamentSize);

        tournament.individuals.clear();
        for (uint8_t i = 0; i < tournamentSize; i++) {
            tournament.individuals.push_back(individuals[(std::rand() % populationSize)]);
        }

        return tournament.getFittest();
    }

};

class Simulator {
public:
    std::map<int, float> scoreHistory;

    void scorePopulation(Population& pop) {
        for (int i = 0; i < pop.populationSize; ++i) {
            int h = pop.individuals[i].hash();
            auto search = scoreHistory.find(h);
            if (search == scoreHistory.end()) {
                playGame(pop.individuals[i], GAMES_POP);
            } else {
                pop.individuals[i].fitness = search->second;
            }
        }
    }

    float exec(const std::string& cmd) {
        std::array<char, 256> buffer;
        std::string result;
        std::shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose);
        if (!pipe) throw std::runtime_error("popen() failed!");
        float res = 0;
        int gameNb = 0;
        while (!feof(pipe.get())) {
            if (fgets(buffer.data(), 256, pipe.get()) != NULL) {
                result += buffer.data();
                char* p = strstr(buffer.data(), "%");
                if (p != nullptr) {
                    ++gameNb;
                    std::string a(p - 6, p);
                    res = std::stof(a);
                    std::cerr << "Game " << gameNb << " : " << res << "%\n";
                }
                if (strstr(buffer.data(), "End of games") > 0) {
                    break;
                }
            }
        }
        return res;
    }

    void playGame(Individual& indiv, int nb) {
        std::ostringstream oss;

        oss << BRUTAL_BINARY << " -n " << nb << " -p1 '" << CSB_BINARY;

        indiv.getValues(oss);

        oss << "' -p2 '" << CSB_BINARY;
        for (int i = 0; i < MAX_SCORES; ++i) {
            oss << " " << SCORE_BASES[i];
        }
        oss << "'";

        std::cerr << nb << " games on params :";
        indiv.getValues(std::cerr);
        std::cerr << std::endl;

        indiv.fitness = exec(oss.str());
        scoreHistory[indiv.hash()] = indiv.fitness;
    }
};

bool doIt(Simulator& sim, Population& pop) {
    bool rescore = false;
    sim.scorePopulation(pop);
    // sort population by fitness & test all >= SCORE upgrade% to replace release
    std::sort(pop.individuals.begin(), pop.individuals.end(), [](const Individual& a, const Individual& b){
        return a.fitness < b.fitness;
    });

    for (int i = pop.individuals.size() - 1; i >= 0; --i) {
        if (pop.individuals[i].fitness < MIN_SCORE_FOR_UPGRADE) break;

        // try if it's a real cool one
        sim.playGame(pop.individuals[i], GAMES_VAL);
        if (pop.individuals[i].fitness >= MIN_SCORE_FOR_UPGRADE) { // APPROVED
            Individual test = pop.individuals[i];
            test.getValues(std::cout);
            std::cout << "\nrelease update with score " << test.fitness << std::endl;
            memcpy(SCORE_BASES, test.genes, sizeof(test.genes));
            // reset all population
            rescore = true;
            break;
        }

    }
    return rescore;
}

int main(int argc, char** argv) {

    if (argc == MAX_SCORES + 1) {
        std::cout << "override params" << std::endl;;

        for (int i = 0; i < MAX_SCORES;++i) {
            SCORE_BASES[i] = std::stof(argv[i+1]);
        }
    }

    srand(time(0));
    int populationSize = 10;

    Simulator sim;
    Population pop(populationSize);

    while (true) {
        bool rescore = false;
        pop.init(SCORE_BASES);
        sim.scoreHistory.clear();
        sim.scoreHistory[pop.individuals[0].hash()] = 50.0;

        rescore = doIt(sim, pop);

        while (!rescore) {
            pop = pop.evolve();
            rescore = doIt(sim, pop);
        }
    }

    return 0;
}

