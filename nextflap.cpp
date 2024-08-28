#include "utils/utils.h"
#include "parser/parsedTask.h"
#include "preprocess/preprocess.h"
#include "grounder/grounder.h"
#include "sas/sasTranslator.h"
#include "planner/plannerSetting.h"
#include "planner/z3Checker.h"
#include "planner/printPlan.h"
#include "parser/parser.h"

/********************************************************/
/* Oscar Sapena Vercher - DSIC - UPV                    */
/* April 2022                                           */
/********************************************************/
/* Main method: parses the command-line arguments and   */
/* launches the planner.                                */
/********************************************************/

#define OPTIMIZE_MAKESPAN true
// #define _DEBUG true

struct PlannerParameters {
    float total_time;
    char* domainFileName;
    char* problemFileName;
    bool generateGroundedDomain;
    bool keepStaticData;
    bool noSAS;
    bool generateMutexFile;
    clock_t startTime;
    PlannerParameters() :
        total_time(0), domainFileName(nullptr), problemFileName(nullptr), generateGroundedDomain(false),
        keepStaticData(false), noSAS(false), generateMutexFile(false), startTime(clock()) {
    }
};

// Prints the command-line arguments of the planner
void printUsage() {
    cout << "Usage: NextFLAP <domain_file> <problem_file> [-ground] [-static] [-mutex]" << endl;
    cout << " -ground: generates the GroundedDomain.pddl and GroundedProblem.pddl files." << endl;
    cout << " -static: keeps the static data in the planning task." << endl;
    cout << " -nsas: does not make translation to SAS (finite-domain variables)." << endl;
    cout << " -mutex: generates the mutex.txt file with the list of static mutex facts." << endl;
}

// Parses the domain and problem files
void parseStage(PlannerParameters* parameters, std::unique_ptr<ParsedTask> &parsedTask) {
    clock_t t = clock();
    Parser parser;
    parser.parseDomain(parameters->domainFileName);
    parser.parseProblem(parameters->problemFileName, parsedTask);
    float time = toSeconds(t);
    parameters->total_time += time;
    cout << ";Parsing time: " << time << endl;
    return;
}

// Preprocesses the parsed task
void preprocessStage(std::unique_ptr<ParsedTask> & parsedTask, PlannerParameters* parameters, std::unique_ptr<PreprocessedTask> &prepTask) {
    clock_t t = clock();
    Preprocess preprocess(parsedTask);
    preprocess.preprocessTask(prepTask);
    float time = toSeconds(t);
    parameters->total_time += time;
    //cout << prepTask->toString() << endl;
    cout << ";Preprocessing time: " << time << endl;
    return;
}

// Grounder stage of the preprocessed task
void groundingStage(std::unique_ptr<PreprocessedTask> & prepTask,
    PlannerParameters* parameters, std::unique_ptr<GroundedTask> &gTask) {
    clock_t t = clock();
    Grounder grounder(prepTask);
    grounder.groundTask(parameters->keepStaticData, gTask);
    float time = toSeconds(t);
    parameters->total_time += time;
    //cout << gTask->toString() << endl;
    cout << ";Grounding time: " << time << endl;
    if (parameters->generateGroundedDomain) {
        cout << ";" << gTask->actions.size() << " grounded actions" << endl;
        gTask->writePDDLDomain();
        gTask->writePDDLProblem();
    }
    return;
}

// SAS translation stage
void sasTranslationStage(std::unique_ptr<GroundedTask> &gTask, PlannerParameters* parameters, std::shared_ptr<SASTask> sTask) {
    clock_t t = clock();
    SASTranslator translator;
    translator.translate(gTask, parameters->noSAS,
        parameters->generateMutexFile, parameters->keepStaticData, sTask);
    float time = toSeconds(t);
    parameters->total_time += time;
    cout << ";SAS translation time: " << time << endl;
    //cout << sasTask->toString() << endl;
    /*for (SASAction& a : sasTask->actions) {
        cout << sasTask->toStringAction(a) << endl;
    }*/
    return;
}

// Sequential calls to the preprocess stages
void doPreprocess(PlannerParameters* parameters, std::unique_ptr<GroundedTask> &gTask, std::shared_ptr<SASTask> sTask) {
    parameters->total_time = 0;
    std::unique_ptr<ParsedTask> parsedTask;
    std::unique_ptr<PreprocessedTask> prepTask;
    parseStage(parameters, parsedTask);
    if (parsedTask != nullptr) {
        //cout << parsedTask->toString() << endl;
        preprocessStage(parsedTask, parameters, prepTask);
        if (prepTask != nullptr) {
           groundingStage(prepTask, parameters, gTask);
            if (gTask != nullptr) {
                //cout << gTask->toString() << endl;
                sasTranslationStage(gTask, parameters, sTask);
                // delete gTask;
            }
            // delete prepTask;
        }
    }
    return;
}

// Sequential calls to the main planning stages
void startPlanning(PlannerParameters* parameters) {
    std::unique_ptr<GroundedTask> gTask;
    std::shared_ptr<SASTask> sTask = std::make_shared<SASTask>();
    doPreprocess(parameters, gTask, sTask);
    if (sTask == nullptr)
        return;
    clock_t t = clock();
    PlannerSetting planner(sTask);
    std::shared_ptr<Plan> solution;
    float bestMakespan = FLOAT_INFINITY;
    int bestNumSteps = MAX_UINT16;
    do {
        solution = planner.plan(bestMakespan, t);
        float time = toSeconds(t);
        if (solution != nullptr) {
            //cout << ";Checking solution" << endl;
            Z3Checker checker;
            TControVarValues cvarValues;
            float solutionMakespan;

            if (checker.checkPlan(solution, OPTIMIZE_MAKESPAN, &cvarValues)) {
                solutionMakespan = PrintPlan::getMakespan(solution);
                if (solutionMakespan < bestMakespan ||
                    (abs(solutionMakespan - bestMakespan) < EPSILON && solution->g < bestNumSteps)) {
                    PrintPlan::print(solution, &cvarValues);
                    bestMakespan = solutionMakespan;
                    bestNumSteps = solution->g;
                    cout << ";Solution found in " << time << endl;
                    break;
                }
            }
        }
    } while (solution != nullptr);
}

// Main method
int main(int argc, char* argv[]) {
    PlannerParameters parameters;
    if (argc < 3) {
        printUsage();
    }
    else {
        int param = 1;
        while (param < argc) {
            if (argv[param][0] != '-') {
                if (parameters.domainFileName == nullptr)
                    parameters.domainFileName = argv[param];
                else if (parameters.problemFileName == nullptr)
                    parameters.problemFileName = argv[param];
                else {
                    parameters.domainFileName = nullptr;
                    break;
                }
            }
            else {
                if (compareStr(argv[param], "-ground"))
                    parameters.generateGroundedDomain = true;
                else if (compareStr(argv[param], "-static"))
                    parameters.keepStaticData = true;
                else if (compareStr(argv[param], "-nsas"))
                    parameters.noSAS = true;
                else if (compareStr(argv[param], "-mutex"))
                    parameters.generateMutexFile = true;
                else {
                    parameters.domainFileName = nullptr;
                    break;
                }
            }
            param++;
        }
        if (parameters.domainFileName == nullptr || parameters.problemFileName == nullptr) {
            printUsage();
        }
        else {
            startPlanning(&parameters);
        }
    }

    return 0;
}
