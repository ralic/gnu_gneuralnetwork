/* parser.c -- This belongs to gneural_network

   gneural_network is the GNU package which implements a programmable neural network.

   Copyright (C) 2016 Jean Michel Sellier
   <jeanmichel.sellier@gmail.com>
   Copyright (C) 2016 Francesco Maria Virlinzi
   <francesco.virlinzi@gmail.com>
   Copyright (C) 2016 Ray Dillinger
   <bear@sonic.net>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

// read the input script to define the network and its task

#include "includes.h"
#include "parser.h"
#include "network.h"

enum main_token_id {
  _COMMENT,
  _NUMBER_OF_NEURONS,
  _NEURON,
  _NETWORK,
  _WEIGHT_MINIMUM,
  _WEIGHT_MAXIMUM,

  _LOAD_NEURAL_NETWORK,
  _SAVE_NEURAL_NETWORK,

  _ERROR_TYPE,
  _INITIAL_WEIGHTS_RANDOMIZATION,

  _NUMBER_OF_TRAINING_CASES,
  _TRAINING_CASE,
  _TRAINING_METHOD,

  _NUMBER_OF_INPUT_CASES,
  _NETWORK_INPUT,

  _SAVE_OUTPUT,
  _OUTPUT_FILE_NAME,
};


static const char *main_token_n[] = {
  [_COMMENT]				= "#",
  [_NUMBER_OF_NEURONS]			= "NUMBER_OF_NEURONS",
  [_NEURON]				= "NEURON",
  [_NETWORK]				= "NETWORK",
  [_WEIGHT_MINIMUM]			= "WEIGHT_MINIMUM",
  [_WEIGHT_MAXIMUM]			= "WEIGHT_MAXIMUM",
  [_LOAD_NEURAL_NETWORK]		= "LOAD_NEURAL_NETWORK",
  [_SAVE_NEURAL_NETWORK]		= "SAVE_NEURAL_NETWORK",
  [_ERROR_TYPE]				= "ERROR_TYPE",
  [_INITIAL_WEIGHTS_RANDOMIZATION]	= "INITIAL_WEIGHTS_RANDOMIZATION",
  [_NUMBER_OF_TRAINING_CASES]		= "NUMBER_OF_TRAINING_CASES",
  [_TRAINING_CASE]			= "TRAINING_CASE",
  [_TRAINING_METHOD]			= "TRAINING_METHOD",
  [_NUMBER_OF_INPUT_CASES]		= "NUMBER_OF_INPUT_CASES",
  [_NETWORK_INPUT]			= "NETWORK_INPUT",
  [_SAVE_OUTPUT]			= "SAVE_OUTPUT",
  [_OUTPUT_FILE_NAME]			= "OUTPUT_FILE_NAME",
};


const int main_token_count = 17;

enum direction_enum {
  _IN,
  _OUT,
};

static const char *direction_n[] = {
  [_IN]		= "IN",
  [_OUT]	= "OUT",
};
const int direction_count = 2;

static const char *error_names[] = {
    [MSE] = "MSE",
    [ME] = "ME",
};
const int error_name_count = 2;

static int find_id(char *name, const char *type, const char **array, int last)
{
  int id;
  for (id = 0; id < last; ++id)
	if (strcmp(name, array[id]) == 0)
		return id;
  printf(" -> '%s' not supported as '%s' token\n", name, type);
  exit(-1);
}

static int get_positive_number(FILE *fp, const char *token_n)
{
  int ret, num;
  double tmp;

  ret = fscanf(fp, "%lf", &tmp);
  if (ret < 0)
    exit(-1);

  num = (int)(tmp);
  if (tmp != num) {
	printf("%s must be an integer number! (%f)\n", token_n, tmp);
	exit(-1);
  }
  if (num < 0) {
	printf("%s must be a positive number! (%d)\n ", token_n, num);
	exit(-1);
  }
  //printf(" -> %s = %d [OK]\n", token_n, num);
  return num;
}

static int get_strictly_positive_number(FILE *fp, const char *token_n)
{
  int num = get_positive_number(fp, token_n);
  if (num == 0) {
	printf("%s must be a strictly positive number! (%d)\n ", token_n, num);
	exit(-1);
  }
  return num;
}

static const char *switch_n[] = {
  [OFF] = "OFF",
  [ON]  = "ON",
};
static const int switch_n_size = 2;

static int get_switch_value(FILE *fp, const char *name)
{
  int ret;
  char s[128];
  ret = fscanf(fp, "%126s", s);
  if (ret < 0)
    exit(-1);
  return find_id(s, name, switch_n, switch_n_size);
}

static double get_double_number(FILE *fp)
{
  double tmp;
  int ret = fscanf(fp, "%lf", &tmp);
  if (ret < 0)
    exit(-1);
  return tmp;
}

static double get_double_positive_number(FILE *fp, const char *msg)
{
  double tmp = get_double_number(fp);
  if (tmp > 0)
	return tmp;
  printf("%s must be greater than 0!", msg);
  exit(-1);
}

/*
 * sub_neuron_parser: parse the neuron attribute
 * fp: pointer to the script file
 */
static void sub_neuron_parser(network *nn, network_config *config, FILE *fp)
{
  enum neuron_sub_token_id {
	NUMBER_OF_CONNECTIONS,
	ACTIVATION,
	ACCUMULATOR,
	CONNECTION,
  };
  static const char *neuron_sub_token_n[] = {
	[NUMBER_OF_CONNECTIONS] = "NUMBER_OF_CONNECTIONS",
	[ACTIVATION]		= "ACTIVATION",
	[ACCUMULATOR]		= "ACCUMULATOR",
	[CONNECTION]		= "CONNECTION",
  };

  static const char *accumulator_names[] = {
	[LINEAR]		= "LINEAR",
	[LEGENDRE]		= "LEGENDRE",
	[LAGUERRE]		= "LAGUERRE",
	[FOURIER]		= "FOURIER",
  };

  static const char *activation_names[] = {
	[TANH]			= "TANH",
	[EXP]			= "EXP",
	[EXP_SIGNED]            = "EXP_SIGNED",
	[SOFTSIGN]              = "SOFTSIGN",
	[RAMP]                  = "RAMP",
	[SOFTRAMP]              = "SOFTRAMP",
	[ID]			= "ID",
	[POL1]			= "POL1",
	[POL2]			= "POL2",
  };

  const int neuron_sub_token_count = 4;
  const int accumulator_names_count = 4;
  const int activation_names_count = 9;

  int sub_token, sub_num, index, ret;
  char s[128];

  index = get_positive_number(fp, "NEURON");
  if (index > (nn->num_of_neurons -1)) {
	printf("neuron id out of range!\n");
	exit(-1);
  };

  ret = fscanf(fp, "%126s", s);
  if (ret < 0)
    exit(-1);
  sub_token = find_id(s, "NEURON sub-token",
	neuron_sub_token_n, neuron_sub_token_count);

  switch (sub_token) {
  case NUMBER_OF_CONNECTIONS: {
	sub_num = get_positive_number(fp, neuron_sub_token_n[sub_token]);
	network_neuron_set_connection_number(&nn->neurons[index], sub_num);
	printf("NEURON %d NUMBER_OF_CONNECTIONS = %d [OK]\n", index, sub_num);
	}
	break;
  case ACTIVATION: {
	ret = fscanf(fp, "%126s", s);
	int activ = find_id(s, neuron_sub_token_n[sub_token],
		activation_names, activation_names_count);
	nn->neurons[index].activation = activ;
	printf("NEURON %d ACTIVATION = %s\n", index, activation_names[activ]);
	}
	break;
  case ACCUMULATOR: {
	ret = fscanf(fp, "%126s", s);
	int accum = find_id(s,  neuron_sub_token_n[sub_token],
		accumulator_names, accumulator_names_count);
	nn->neurons[index].accumulator = accum;
	printf("NEURON %d ACCUMULATOR = %s\n", index, accumulator_names[accum]);
	};
	break;
  case CONNECTION: {
	int connection_id = get_positive_number(fp, neuron_sub_token_n[sub_token]);
	if (connection_id > (nn->num_of_neurons -1)) {
		printf("the connection index is out of range!\n");
		exit(-1);
	}
	if (connection_id > (nn->neurons[index].num_input-1) ) {
		printf("the connection index is out of range!\n");
		exit(-1);
	}

	int global_neuron_id_2 = get_positive_number(fp, neuron_sub_token_n[sub_token]);
	if (global_neuron_id_2 > (nn->num_of_neurons -1)) {
		printf("the global index of neuron #2 is out of range!\n");
		exit(-1);
	}
	printf("NEURON %d CONNECTION %d %d [OK]\n",
		index, connection_id, global_neuron_id_2);
	nn->neurons[index].connection[connection_id] = &nn->neurons[global_neuron_id_2];
	};
	break;
	/* close NEURON sub-case; */
  }
}

static void sub_network_parser(network *nn, network_config *config, FILE *fp) {
  enum network_sub_token_id {
	NUMBER_OF_LAYERS,
	LAYER,
	ASSIGN_NEURON_TO_LAYER,
  };
  static const char *network_sub_token_n[] = {
	[NUMBER_OF_LAYERS]      = "NUMBER_OF_LAYERS",
	[LAYER]                 = "LAYER",
	[ASSIGN_NEURON_TO_LAYER]= "ASSIGN_NEURON_TO_LAYER",
  };
  const int network_sub_token_count = 3;
  int ret, sub_token;
  char s[128];

  /*
   * parser NETWORK Sub-Token
   */
  ret = fscanf(fp, "%126s", s);
  if (ret < 0)
    exit(-1);
  sub_token = find_id(s, "NETWORK sub-token",
	network_sub_token_n, network_sub_token_count);
  switch (sub_token) {
  case NUMBER_OF_LAYERS: {
	int num_layers = get_positive_number(fp, "NUMBER_OF_LAYERS");
	network_set_layer_number(nn, num_layers);
	printf("NETWORK NUMBER_OF_LAYERS = %d [OK]\n", num_layers);
	}
	break;
  // specify the number of neurons of a layer
  // syntax: NETWORK LAYER ind NUMBER_OF_NEURONS num
  case LAYER: {
	int ind = get_positive_number(fp, "LAYER");
	if (ind > (nn->num_of_layers - 1)) {
		printf("layer index is out of range!\n");
		exit(-1);
	}
	ret = fscanf(fp, "%126s", s);
	if (strcmp(s,"NUMBER_OF_NEURONS") != 0) {
		printf("syntax error!\nNUMBER_OF_NEURONS expected!\n");
		exit(-1);
	}
	int num = get_positive_number(fp, "NUMBER_OF_NEURONS");
	if (num > nn->num_of_neurons) {
		printf("the number of neurons in the layer is grater the the total number of neurons!\n");
		printf("please check your configuration!\n");
		exit(-1);
	}
	nn->layers[ind].num_of_neurons = num;
	printf("NETWORK LAYER %d NUMBER_OF_NEURONS %d [OK]\n", ind, num);
	}
	break;
  // assigns the neurons to the layers
  // syntax: NETWORK ASSIGN_NEURON_TO_LAYER layer_id local_neuron_id global_neuron_id
  case ASSIGN_NEURON_TO_LAYER: {
	int layer_id = get_positive_number(fp, "ASSIGN_NEURON_TO_LAYER 1.");
	if (layer_id > (nn->num_of_layers - 1)) {
		printf("layer index out of range!\n");
		exit(-1);
	}
	int local_neuron_id = get_positive_number(fp, "ASSIGN_NEURON_TO_LAYER 2.");
	if (local_neuron_id > (nn->layers[layer_id].num_of_neurons -1)) {
		printf("local neuron index out of range!\n");
		exit(-1);
	}
	int global_neuron_id = get_positive_number(fp, "ASSIGN_NEURON_TO_LAYER 3.");
	if (global_neuron_id > (nn->num_of_neurons -1)) {
		printf("global neuron index out of range!\n");
		exit(-1);
	}
	printf("NETWORK ASSIGN_NEURON_TO_LAYER %d %d %d [OK]\n",
		layer_id, local_neuron_id, global_neuron_id);
	/* assign only the first-one */
	if (!nn->layers[layer_id].neurons)
	  nn->layers[layer_id].neurons = &nn->neurons[global_neuron_id];
	};
	break;
  default:
	printf("the specified network feature is unknown!\n");
	exit(-1);
	break;
  }

}

static void sub_training_method_parser(network *nn, network_config *config, FILE *fp)
{
  static const char *sub_method_token_n[] = {
	[SIMULATED_ANNEALING]   = "SIMULATED_ANNEALING",
	[RANDOM_SEARCH]         = "RANDOM_SEARCH",
	[GRADIENT_DESCENT]      = "GRADIENT_DESCENT",
	[GENETIC_ALGORITHM]     = "GENETIC_ALGORITHM",
	[MSMCO]                 = "MSMCO",
  };
  const int sub_method_token_count = 5;

  int ret, method_id;
  char s[128];
  ret = fscanf(fp, "%126s", s);
  if (ret < 0)
    exit(-1);

  method_id = find_id(s, "TRAINING_METHOD", sub_method_token_n, sub_method_token_count);
  switch (method_id) {
  case SIMULATED_ANNEALING: {
	// simulated annealing
	// syntax: verbosity mmax nmax kbtmin kbtmax accuracy
	// where
	// verbosity = ON/OFF
	// mmax      = outer loop - number of effective temperature steps
	// nmax      = inner loop - number of test configurations
	// kbtmin    = effective temperature minimum
	// kbtmax    = effective temperature maximum
	// accuracy  = numerical accuracy
	int verbosity = get_switch_value(fp, "verbosity");
	int mmax = get_positive_number(fp, "simulated annealing mmax");
	if (mmax < 2) {
		printf("MMAX must be greater than 1!\n");
		exit(-1);
	}
	int nmax = get_positive_number(fp, "simulated annealing nmax");
	double kbtmin = get_double_number(fp);
	double kbtmax = get_double_number(fp);;
	if (kbtmin >= kbtmax) {
		printf("KBTMIN must be smaller then KBTMAX!\n");
		exit(-1);
	}
	double eps = get_double_positive_number(fp, "ACCURACY");
	printf("TRAINING METHOD = SIMULATED ANNEALING %d %d %g %g %g [OK]\n",
		mmax, nmax, kbtmin, kbtmax, eps);
	config->optimization_type = SIMULATED_ANNEALING;
	config->verbosity = verbosity;
	config->mmax = mmax;
	config->nmax = nmax;
	config->kbtmin = kbtmin;
	config->kbtmax = kbtmax;
	config->accuracy = eps;
	};
	break;
  // random search
  // syntax: verbosity nmax accuracy
  // verbosity = ON/OFF
  // nmax      = maximum number of random attempts
  // accuracy  = numerical accuracy
  case RANDOM_SEARCH: {
	int verbosity = get_switch_value(fp, "verbosity");
	int nmax = get_positive_number(fp, "random search nmax");
	double eps = get_double_positive_number(fp, "ACCURACY");
	printf("OPTIMIZATION METHOD = RANDOM SEARCH %d %g [OK]\n",nmax,eps);
	config->verbosity = verbosity;
	config->optimization_type = RANDOM_SEARCH;
	config->nmax = nmax;
	config->accuracy = eps;
	}
	break;
  // gradient descent
  // syntax: verbosity nxw maxiter gamma accuracy
  // where
  // verbosity = ON/OFF
  // nxw       = number of cells in one direction of the weight space
  // maxiter   = maximum number of iterations
  // gamma     = step size
  // accuracy  = numerical accuracy
  case GRADIENT_DESCENT: {
	int verbosity = get_switch_value(fp, "verbosity");
	int nxw      = get_positive_number(fp, "gradient descent nxw");
	int maxiter  = get_positive_number(fp, "gradient descent MAXITER");
	double gamma = get_double_positive_number(fp, "GAMMA");
	double eps = get_double_positive_number(fp, "ACCURACY");
	printf("OPTIMIZATION METHOD = GRADIENT DESCENT %d %d %g %g [OK]\n",
		nxw, maxiter, gamma, eps);
	config->verbosity = verbosity;
	config->optimization_type = GRADIENT_DESCENT;
	config->nxw = nxw;
	config->maxiter = maxiter;
	config->gamma = gamma;
	config->accuracy = eps;
	};
	break;
  // genetic algorithm
  // syntax: verbosity nmax npop rate accuracy
  // where:
  // verbosity = ON/OFF
  // nmax      = number of generations
  // npop      = number of individuals per generation
  // rate      = rate of change between one generation and the parent
  // accuracy  = numerical accuracy
  case GENETIC_ALGORITHM: {
	int verbosity = get_switch_value(fp, "verbosity");
	int nmax = get_positive_number(fp, "genetic algorithm nmax");
	int npop = get_positive_number(fp, "genetic algorithm npop");
	double rate = get_double_positive_number(fp, "RATE");
	double eps = get_double_positive_number(fp, "ACCURACY");
	printf("OPTIMIZATION METHOD = GENETIC ALGORITHM %d %d %g %g [OK]\n",
		nmax, npop, rate, eps);
	config->verbosity = verbosity;
	config->optimization_type = GENETIC_ALGORITHM;
	config->nmax = nmax;
	config->npop = npop;
	config->rate = rate;
	config->accuracy = eps;
	};
	break;
  // multi-scale Monte Carlo algorithm
  // syntax: verbosity mmax nmax rate
  // where:
  // verbosity = ON/OFF
  // mmax      = number of Monte Carlo outer iterations
  // nmax      = number of MC inner iterations
  // rate      = rate of change of the space of search at each iteration
  case MSMCO: {
	int verbosity = get_switch_value(fp, "verbosity");
	int mmax = get_positive_number(fp, "multi-scale Monte Carlo mmax");
	int nmax = get_positive_number(fp, "multi-scale Monte Carlo nmax");
	double rate = get_double_positive_number(fp, "RATE");
	printf("OPTIMIZATION METHOD = MULTI-SCALE MONTE CARLO OPTIMIZATION %d %d %g [OK]\n",
		mmax, nmax, rate);
	config->verbosity = verbosity;
	config->optimization_type = MSMCO;
	config->mmax = mmax;
	config->nmax = nmax;
	config->rate = rate;
	};
	break;
  default:
	break;
  }
}

void parser(network *nn, network_config *config, FILE *fp){
 int ret;
 char s[256];
 double tmp;
 unsigned int token_id;

 printf("\n\
=========================\n\
processing the input file\n\
=========================\n");
 do{
  // read the current row
     ret = fscanf(fp,"%254s", s);
     if (ret == 0)return;

  token_id = find_id(s, "Token", main_token_n, main_token_count);

  switch (token_id) {

  case _COMMENT:
	fgets(s, 80, fp);
	printf("COMMENT ---> %s", s);
	break;

  case _NUMBER_OF_NEURONS:
	network_set_neuron_number(nn,
	    get_strictly_positive_number(fp, "TOTAL NUMBER OF NEURONS"));
	printf("TOTAL NUMBER OF NEURONS = %d [OK]\n", nn->num_of_neurons);
	break;

  case _NEURON:
	sub_neuron_parser(nn, config, fp);
	break;

  case _NETWORK:
	sub_network_parser(nn, config, fp);
	break;

  case _NUMBER_OF_TRAINING_CASES: {
	int ncase = get_strictly_positive_number(fp, main_token_n[token_id]);
	if (ncase > MAX_TRAINING_CASES) {
	        printf("NUMBER_OF_TRAINING_CASES is too large!\n");
	        printf("please increase MAX_TRAINING_CASES and recompile!\n");
	        exit(-1);
	        }
	printf("NUMBER_OF_TRAINING_CASES = %d [OK]\n", ncase);
	config->num_cases = ncase;
	}
	break;

  // specify the training cases for supervised learning
  // syntax: TRAINING_CASE IN case_index neuron_index _connection_index value
  // syntax: TRAINING_CASE OUT case_index neuron_index value
  case _TRAINING_CASE: {
	ret = fscanf(fp, "%254s", s);
	int direction = find_id(s, main_token_n[token_id],
		direction_n, direction_count);
	switch (direction) {
	case _IN: {
		int ind = get_positive_number(fp, "training data index");

		if (ind > (config->num_cases -1)) {
			printf("training data index out of range!\n");
			exit(-1);
			}
		int neu = get_positive_number(fp, "TRAINING_CASE neuron index");

		if (neu > (nn->num_of_neurons -1)) {
			printf("TRAINING_CASE IN neuron index out of range!\n");
			exit(-1);
			}

		int conn = get_positive_number(fp, "TRAINING_CASE connection index");
		if (conn > (nn->neurons[neu].num_input -1)) {
			printf("TRAINING_CASE connection index out of range!\n");
			exit(-1);
		}
		tmp = get_double_number(fp);
		printf("TRAINING_CASE IN %d %d %d %f [OK]\n",
			ind, neu, conn, tmp);
		config->cases_x[ind][neu][conn] = tmp;
		};
		break;
	case _OUT: {
		int ind = get_positive_number(fp, "training data index");
		if (ind > (config->num_cases -1)) {
			printf("training data index out of range!\n");
			exit(-1);
			}
		int neu = get_positive_number(fp, "TRAINING_CASE OUT neuron index");
		if (neu > (nn->num_of_neurons -1)) {
			printf("TRAINING_CASE OUT neuron index out of range!\n");
			exit(-1);
			}
		tmp = get_double_number(fp);
		printf("TRAINING_CASE OUT %d %d %f [OK]\n", ind, neu, tmp);
		config->cases_y[ind][neu] = tmp;
		}
	} /* close switch (direction) */
	};
	break;

    case _WEIGHT_MINIMUM:
	config->wmin = get_double_number(fp);;
	printf("WEIGHT_MINIMUM = %f [OK]\n", config->wmin);
	break;

    case _WEIGHT_MAXIMUM:
	config->wmax = get_double_number(fp);
	printf("WEIGHT_MAXIMUM = %f [OK]\n", config->wmax);
	break;
    // specify the training method
    // syntax: TRAINING_METHOD method values (see below)..
    case _TRAINING_METHOD:
	sub_training_method_parser(nn, config, fp);
	break;

  // specify if some output has to be saved
  // syntax: SAVE_OUTPUT ON/OFF
  case _SAVE_OUTPUT:
	config->save_output = get_switch_value(fp, "save_output");
	printf("SAVE_OUTPUT %s [OK]\n", switch_n[config->save_output]);
	break;

  // specify the output file name
  // syntax: OUTPUT_FILE_NAME filename
  case _OUTPUT_FILE_NAME: {
	ret = fscanf(fp, "%254s", s);
	config->output_file_name = malloc(strlen(s) + 1);
	strcpy(config->output_file_name, s);
        printf("OUTPUT FILE NAME = %s [OK]\n", config->output_file_name);
        break;
  // specify the number of cases for the output file
  // syntax: NUMBER_OF_INPUT_CASES num
  case _NUMBER_OF_INPUT_CASES: {
       int num = get_strictly_positive_number(fp, "NUMBER_OF_INPUT_CASES");
       if (num > MAX_NUM_CASES) {
               printf("NUMBER_OF_INPUT_CASES is too large!\n");
               printf("please increase MAX_NUM_CASES and recompile!\n");
               exit(-1);
               }
	printf("NUMBER OF INPUT CASES = %d [OK]\n", num);
	config->num_of_cases = num;
	};
        break;
  // specify the input cases for the output file
  // syntax: NETWORK_INPUT case_id neuron_id conn_id val
  case _NETWORK_INPUT: {
	int num = get_positive_number(fp, "NETWORK_INPUT case index");
	if (num > (config->num_of_cases - 1)) {
		printf("NETWORK_INPUT case index out of range!\n");
		exit(-1);
		}

	int neu = get_positive_number(fp, "NETWORK_INPUT neuron index");
	if (neu > nn->layers[0].num_of_neurons - 1) {
		printf("NETWORK_INPUT neuron index out of range!\n");
		exit(-1);
		}
	int conn = get_positive_number(fp, "NETWORK_INPUT connection index");
	if (conn > (nn->neurons[neu].num_input -1)) {
		printf("NETWORK_INPUT connection index out of range!\n");
		exit(-1);
		}
	double val = get_double_number(fp);
	printf("NETWORK INPUT CASE #%d %d %d = %g [OK]\n",
		num, neu, conn, val);
	config->output_x[num][neu][conn]=val;
	};
	break;
  // save a neural network (structure and weights) in the file network.dat
  // at the end of the training process
  // syntax: SAVE_NEURAL_NETWORK
  case _SAVE_NEURAL_NETWORK: {
	ret = fscanf(fp, "%254s", s);
	config->save_network_file_name = malloc(strlen(s) + 1);
	strcpy(config->save_network_file_name, s);
	config->save_neural_network = ON;
	printf("SAVE NEURAL NETWORK to %s [OK]\n", s);
        };
	break;

  // load a neural network (structure and weights) from the file network.dat
  // at the begining of the training process
  // syntax: LOAD_NEURAL_NETWORK
  case _LOAD_NEURAL_NETWORK: {
	ret = fscanf(fp, "%254s", s);
	config->load_network_file_name = malloc(strlen(s) + 1);
	strcpy(config->load_network_file_name, s);
	config->save_neural_network = ON;
	printf("LOAD NEURAL NETWORK from %s [OK]\n", s);
	};
	break;

  // perform a random initialization of the weights
  // syntax: INITIAL_WEIGHTS_RANDOMIZATION ON/OFF
  case _INITIAL_WEIGHTS_RANDOMIZATION: {
	int flag = get_switch_value(fp, main_token_n[token_id]);
	config->initial_weights_randomization = flag;
	printf("INITIAL_WEIGHTS_RANDOMIZATION = %s [OK]\n", switch_n[flag]);
	};
	break;
  // specify the error function for the training process
  // syntax: ERROR_TYPE MSE/ME
  case _ERROR_TYPE: {
	ret = fscanf(fp, "%254s", s);
	int errtype = find_id(s, main_token_n[token_id],
		error_names, error_name_count);
	config->error_type = errtype;
	printf("ERROR_TYPE = %s [OK]\n", error_names[errtype]);
	};
	break;
    } /* close switch(token_id) */
  }
  sprintf(s,""); // empty the buffer
 } while (!feof(fp));
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Below this point, all the definitions are provided for nnet (second build target with new network representation)    //
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


static const char* acctokens[ACCUMCOUNT] = {ACCTOKENS};
static const char* outtokens[OUTPUTCOUNT] = {OUTTOKENS};

// output parse error msg and halt.  To free any dynamically allocated buffers, pass their address as free1. Otherwise pass NULL.
void ErrStopParsing(struct slidingbuffer *bf, const char *msg, void *free1){
    assert (msg != NULL);
    free(free1);    fflush(stdout);  fclose(bf->input);
    if (bf == NULL) fprintf(stderr, "\n %s\n",msg);
    else fprintf(stderr, "\nLine %d Col %d : %s\n", bf->line, bf->col, msg);
    exit(1);
}

// add a warning to the slidingbuffer - don't output it yet.
void AddWarning(struct slidingbuffer *bf, const char *msg){
    assert(msg != NULL);
    int warnlen = bf->warnings==NULL ? 0 : strlen(bf->warnings); int size = warnlen+strlen(msg)+50;
    bf->warnings = (char *)realloc(bf->warnings, size);
    char *cursor = &(bf->warnings[warnlen]);
    snprintf(cursor, size, "Line %3d Col %3d : %s\n", bf->line, bf->col, msg);
}

// print all warning messages so far saved in the slidingbuffer.
void PrintWarnings(struct slidingbuffer *bf){assert(bf != NULL); if (bf->warnings != NULL) fprintf(stderr, "\n%s", bf->warnings);}

// Ensure that there are at least min characters available in buffer.  Return 0 on fail, #chars available on success.
int ChAvailable(struct slidingbuffer *bf, int min){
    assert(bf != NULL);
    if (bf->head - bf->end < min)
	while(!feof(bf->input) && (bf->head - bf->end < BFLEN ))bf->buffer[(bf->head++)%BFLEN]=fgetc(bf->input);
    if (bf->head - bf->end < min) return (0);
    return(bf->head - bf->end);
}

// tell what the next character is without accepting it.
int NextCh(struct slidingbuffer *bf){assert(bf != NULL); if (ChAvailable(bf,1)) return(bf->buffer[bf->end % BFLEN]); else return(0); }

// accept the specified number of characters updating the line & column counts.  Return #characters accepted.  Error messages give line numbers, so for verbose
// debugging this routine outputs lines accepted with line numbers.
int AcceptCh(struct slidingbuffer *bf, int len){
    assert(bf != NULL);  assert(len >= 0);
    char next;
    static int AnnouncedLineZero = 0;
    if (!AnnouncedLineZero++)printf("    0: ");
    if (ChAvailable(bf, len)) {
	for (int cn = 0; cn < len; cn++){
	    next = NextCh(bf);
	    if ('\n' == next){printf("\n %4d: ", ++(bf->line)); bf->col = 0;} else {printf("%c",next); bf->col++;}
	    bf->end++;
	}
	return (len);
    }
    return(0);
}

// return tokenlength iff a given token is ready to read from bf, 0 otherwise.
int TokenAvailable(struct slidingbuffer *bf, const char *token){
    assert(bf != NULL); assert (token != NULL);
    int goal = strlen(token);
    if (!ChAvailable(bf,goal)) return(0);
    for (int ct = 0; ct < goal; ct++)if (bf->buffer[(bf->end+ct)%BFLEN] != token[ct]) return 0;
    return(goal);
}

// My duck is on fire.

// accept a given token iff it is available at beginning of input.  Return #characters accepted
int AcceptToken(struct slidingbuffer *bf, const char *token){assert(bf != NULL); assert (token != NULL); return(AcceptCh(bf, TokenAvailable(bf, token)));}

// skip as much whitespace as there is available.
void SkipWhiteSpace(struct slidingbuffer *bf){assert(bf != NULL); while (ChAvailable(bf,1) && isspace(NextCh(bf))) AcceptCh(bf, 1);}

// skip a comment iff a comment is at head of input. (comments are everything between # and EOL.)
void SkipComment(struct slidingbuffer *bf){assert(bf != NULL); if (TokenAvailable(bf, "#")) while(NextCh(bf) != '\n') AcceptCh(bf, 1);}

// read past all whitespace and comments to the next non-skipped character.
void SkipToNext(struct slidingbuffer *bf){assert(bf != NULL); do{SkipWhiteSpace(bf); SkipComment(bf); } while(isspace(NextCh(bf)) || TokenAvailable(bf, "#"));}

// returns nonzero iff a number is available starting at the next character.
int NumberAvailable(struct slidingbuffer *bf){assert(bf != NULL); int next = NextCh(bf);return (next == '+' || next == '-' || isdigit(next));}

// Read and return the number.  Controlled fail with helpful message if none available or wrong syntax. You must call 'NumberAvailable' to check validity first.
int ReadInteger(struct slidingbuffer *bf){
    assert(bf != NULL);     assert(NumberAvailable(bf));
    static const int maxlen = (int)(log10(INT_MAX)) + 2;
    char buf[maxlen];
    ChAvailable(bf,2);
    int count = 0; int negate = (NextCh(bf) == '-');
    if (negate || (NextCh(bf) == '+')) AcceptCh(bf,1);
    if (!isdigit(NextCh(bf))) ErrStopParsing(bf, "Expected Integer.",NULL);
    while(count <  maxlen && ChAvailable(bf,1) && isdigit(NextCh(bf))){buf[count++] = NextCh(bf); AcceptCh(bf,1);}
    if ('.' == NextCh(bf)) ErrStopParsing(bf, "Found Decimal Fraction, Expected Integer.",NULL);
    if (count >= maxlen-2) ErrStopParsing(bf, "Integer too long.",NULL);
    buf[count] = 0;
    return( (1-(2*negate)) * atoi(buf));
}

// Read and return the number.  Controlled fail with helpful message if none available or wrong syntax.  You must call 'NumberAvailable' first.
double ReadFloatingPoint(struct slidingbuffer *bf){
    assert(bf != NULL);    assert(NumberAvailable(bf));
    static const int maxlen = 1085; // max decimal length for (negative, denormalized, 64-bit) double is 1079!!  That's CRAZY!
    char buf[maxlen]; int count = 0; int before = 0; int after = 0;
    if (!TokenAvailable(bf,"+") && !TokenAvailable(bf,"-") && !isdigit(NextCh(bf))) return (0);
    if (ChAvailable(bf,1) && (NextCh(bf) == '-' || NextCh(bf) == '+')) {buf[count++] = NextCh(bf); AcceptCh(bf,1);}
    if (!ChAvailable(bf,1) || !isdigit(NextCh(bf))) ErrStopParsing(bf, "Expected Floating Point Value.",NULL);
    while (ChAvailable(bf,1) && isdigit(NextCh(bf)) && count < maxlen-1){before = 1; buf[count++] = NextCh(bf); AcceptCh(bf,1);}
    if (!ChAvailable(bf,1) || NextCh(bf) != '.') ErrStopParsing(bf, "Floating-point values must have a decimal point.",NULL);
    else {buf[count++] = NextCh(bf); AcceptCh(bf,1);}
    while (ChAvailable(bf,1) && isdigit(NextCh(bf)) && count < maxlen-1) {after = 1; buf[count++] = NextCh(bf); AcceptCh(bf,1);}
    if (before == 0 || after == 0) ErrStopParsing(bf,"Floating-point values must have digits before and after decimal.",NULL);
    if (ChAvailable(bf,2) && count < (maxlen-2) && (AcceptToken(bf, "e") || AcceptToken(bf,"E"))){
	buf[count++] = 'e';
	if (count < maxlen-2 && (NextCh(bf) == '+' || NextCh(bf) == '-')) {buf[count++] = NextCh(bf); AcceptCh(bf,1);}
	if (!isdigit(NextCh(bf))) ErrStopParsing(bf,"Scientific notation floats must have digits in exponent.",NULL);
	else while (count < maxlen-2 && ChAvailable(bf,1) && isdigit(NextCh(bf))) {buf[count++] = NextCh(bf); AcceptCh(bf,1);}
    }
    if (count >= maxlen-2) ErrStopParsing(bf, "Floating-point value is too long.",NULL);
    buf[count] = 0;    int non0digits = 0;
    for (;--count >= 0;) non0digits += buf[count]=='e' ? -non0digits : (int)(isdigit(buf[count])&&buf[count]!='0');
    double retval = atof(buf);
    if (retval == 0.0 && non0digits != 0) ErrStopParsing(bf,"Nonzero float in source was rounded to zero on read.",NULL);
    if (retval == HUGE_VAL) ErrStopParsing(bf,"Float value in source exceeds float range.",NULL);
    return(retval);
}

// returns zero for failure, nonzero for success.
int ReadIntSpan(struct slidingbuffer *bf, int *start, int *end){
    assert(bf != NULL); assert (start != NULL);
    SkipToNext(bf);    if (!AcceptToken(bf,"{")) return 0;
    SkipToNext(bf);    if (!NumberAvailable(bf)) ErrStopParsing(bf, "Integer span starts with non-numeric token.  Integer Expected.",NULL);
    *start = ReadInteger(bf);
    SkipToNext(bf);    if (!NumberAvailable(bf)) ErrStopParsing(bf, "Integer span ends with non-numeric token.  Integer Expected.",NULL);
    *end = ReadInteger(bf);
    SkipToNext(bf);    if (!AcceptToken(bf,"}")) ErrStopParsing(bf, "Integer spans must end with \'}\'.",NULL);
    if (*start > *end) {int shuffle = *end; *end = *start; *start = shuffle;}
    return (1);
}

// read a weight matrix.  Return 0 for failure, 1 for success
int ReadWeightMatrix(struct slidingbuffer *bf, double *target, int size){
    assert(bf != NULL); assert (target != NULL); assert (size > 0);
    int index;
    SkipToNext(bf); if (!AcceptToken(bf,"[")) return 0;
    for (index = 0; index < size; index++){
	SkipToNext(bf);  if (NumberAvailable(bf)) target[index] = ReadFloatingPoint(bf);
	else if (TokenAvailable(bf, "]")) ErrStopParsing(bf,"Weight matrix has too few weights.",NULL);
	else ErrStopParsing(bf, "Non-numeric token in weight matrix.",NULL);
    }
    SkipToNext(bf);
    if (NumberAvailable(bf)) ErrStopParsing(bf, "Weight matrix has too many weights.",NULL);
    if (!AcceptToken(bf,"]")) ErrStopParsing(bf,"']' expected at end of weight matrix.",NULL);
    return 1;
}

#define WARNSIZE 256
int ReadCreateNodeStmt(struct slidingbuffer *bf, struct nnet *net){
    assert(bf != NULL); assert (net != NULL);
    int in_hid_out = 0;
    int NumToCreate = 0;
    int Accum = 0;
    int Transfer = 0;
    int unitwidth = 1;
    char warnstring[WARNSIZE];
    SkipToNext(bf);    if (!TokenAvailable(bf,"CreateInput") && !TokenAvailable(bf,"CreateHidden") && !TokenAvailable(bf,"CreateOutput")) return 0;
    if (AcceptToken(bf,     "CreateInput")) in_hid_out = 1;
    else if (AcceptToken(bf,"CreateHidden")) in_hid_out = 2;
    else if (AcceptToken(bf,"CreateOutput")) in_hid_out = 3;
    else assert (1 == 0); // unhandled case.
    SkipToNext(bf);    if (!AcceptToken(bf, "(")) ErrStopParsing(bf, "Expected Open Parenthesis",NULL);
    SkipToNext(bf);    if (!NumberAvailable(bf)) ErrStopParsing(bf, "First argument of a node creation statement must be an integer.",NULL);
    NumToCreate = ReadInteger(bf);
    if (NumToCreate <= 0) ErrStopParsing(bf, "It doesn't make sense to create less than one node.", NULL);
    SkipToNext(bf);    if (!AcceptToken(bf,",")) ErrStopParsing(bf, "Expected comma between arguments.",NULL);SkipToNext(bf);
    for (Accum = 0; Accum < ACCUMCOUNT && !AcceptToken(bf, acctokens[Accum]); Accum++);
    if (Accum == ACCUMCOUNT){
	int ct = 0; int printed = snprintf(warnstring, WARNSIZE,"Expected the name of an accumulator function:");
	for (ct = 0; ct < ACCUMCOUNT && printed + strlen(acctokens[ct])+5 < WARNSIZE; ct++)
	    printed += snprintf(&(warnstring[printed]), WARNSIZE-printed, " %s", acctokens[ct]);
	snprintf(&(warnstring[printed]),WARNSIZE-printed, ".");
	assert (ct == ACCUMCOUNT); // if it's not we didn't have enough space in warnstring for the names of our input functions.
	ErrStopParsing(bf, warnstring, NULL);
    }
    SkipToNext(bf); if (!AcceptToken(bf,",")) ErrStopParsing(bf, "Expected comma between arguments.",NULL);
    SkipToNext(bf); for (Transfer = 0; Transfer < OUTPUTCOUNT && !AcceptToken(bf, outtokens[Transfer]); Transfer++);
    if (Transfer == OUTPUTCOUNT){
	int ct = 0; int printed = snprintf(warnstring, WARNSIZE,"Expected the name of an activation function:");
	for (ct = 0; ct < OUTPUTCOUNT && printed + strlen(outtokens[ct])+5 < WARNSIZE; ct++)
	    printed += snprintf(&(warnstring[printed]), WARNSIZE-printed, " %s", outtokens[ct]);
	snprintf(&(warnstring[printed]), WARNSIZE-printed, ".");
	assert (ct == OUTPUTCOUNT); // if it's not we didn't have enough space in warnstring for the names of our activation functions.
	ErrStopParsing(bf,warnstring,NULL);
    }
    if (Transfer >= SINGLEOUTPUTS){
	if (!AcceptToken(bf,","))
	    ErrStopParsing(bf, "Expected comma between arguments.  Parallel activation functions require the unit width in nodes as a fourth argument.",NULL);
	SkipToNext(bf);   if (!NumberAvailable(bf)) ErrStopParsing(bf, "Expected Integer.",NULL);
	unitwidth = ReadInteger(bf);	SkipToNext(bf);
    }
    SkipToNext(bf); if (!AcceptToken(bf, ")")) ErrStopParsing(bf, "Expected Close Parenthesis",NULL);
    switch(in_hid_out){
    case 1: AddInputNodes(net, NumToCreate, Transfer, Accum, unitwidth );
	if (net->nodecount > net->inputcount){
	    snprintf(warnstring, WARNSIZE,  "Warning: New Input nodes are numbered %d to %d.  Existing hidden and output nodes have been renumbered %d to %d.",
		     net->inputcount - NumToCreate, net->inputcount-1, net->inputcount, net->nodecount-1);
	    AddWarning(bf,warnstring);
	}
	break;
    case 2: AddHiddenNodes(net, NumToCreate, Transfer, Accum, unitwidth);
	if (net->outputcount > 0){
	    snprintf(warnstring, WARNSIZE, "Warning: New Hidden nodes are numbered %d to %d.  Existing output nodes have been renumbered %d to %d.",
		    net->nodecount-net->outputcount-NumToCreate, net->nodecount-net->outputcount-1, net->nodecount-net->outputcount, net->nodecount-1 );
	    AddWarning(bf,warnstring);
	}
	break;
    case 3: AddOutputNodes(net, NumToCreate, Transfer, Accum, unitwidth); break;
    default: assert(1 == 0); // Unhandled case.
    }
    return (1);
}

int ReadConnectStmt(struct slidingbuffer *bf, struct nnet *net){
    assert(bf != NULL); assert (net != NULL);
    int firstlow;             int secondlow;
    int firsthigh;            int secondhigh;
    double weight = 0.0;      int imm_rnd_matrix = 0;
    int weightcount = 0;      double *weightlist = NULL;
    SkipToNext(bf);     if (!AcceptToken(bf,"Connect")) return 0;
    SkipToNext(bf);     if (!AcceptToken(bf, "(")) ErrStopParsing(bf,"Expected Open Parenthesis",NULL);
    SkipToNext(bf);     if (TokenAvailable(bf,"{"))	ReadIntSpan(bf, &firstlow, &firsthigh);
    else if (NumberAvailable(bf)) firstlow = firsthigh = ReadInteger(bf);
    else ErrStopParsing(bf,"Expected Integer or '{'.", NULL);
    SkipToNext(bf);     if (TokenAvailable(bf,"{")) ReadIntSpan(bf, &secondlow, &secondhigh);
    else if (NumberAvailable(bf)) secondlow = secondhigh = ReadInteger(bf);
    else ErrStopParsing(bf,"Expected Integer or '{'.", NULL);
    SkipToNext(bf);     if (NumberAvailable(bf)) weight = ReadFloatingPoint(bf);
    else if (AcceptToken(bf,"Randomize")) imm_rnd_matrix = 1;
    else if (TokenAvailable(bf,"[")){
	weightcount = (1 + firsthigh - firstlow) * (1 + secondhigh - secondlow);
	weightlist = (double *) malloc(sizeof(double) * weightcount);
	if (weightlist == NULL) ErrStopParsing(bf,"Runtime error: allocation failure in ReadConnectStmt.",NULL);
	imm_rnd_matrix = 2;
	ReadWeightMatrix(bf, weightlist, weightcount);
    }
    else ErrStopParsing(bf, "Expected floating point value, 'Randomize', or '['",NULL);
    SkipToNext(bf);    if (!AcceptToken(bf,")")) ErrStopParsing(bf, "Expected Close Parenthesis", weightlist);
    if (firstlow < 0 || secondlow < 0) ErrStopParsing(bf, "Connect Statement contains negative node ID.", weightlist);
    if (secondlow == 0) ErrStopParsing(bf,"Connect Statement names bias node as destination.", weightlist);
    if (firsthigh >= net->nodecount || secondhigh >= net->nodecount)
	ErrStopParsing(bf, "Connect statement contains node index greater than network's node count.", weightlist);
    switch( imm_rnd_matrix){
    case 0: AddConnections(net, firstlow, firsthigh, secondlow, secondhigh, &weight); break;
    case 1: AddRandomizedConnections(net, firstlow, firsthigh, secondlow, secondhigh); break;
    case 2: AddConnections(net, firstlow, firsthigh, secondlow, secondhigh, weightlist); break;
    default: ErrStopParsing(bf, "Program Error: unhandled case in ReadConnectStmt.", weightlist); // This never happens
    }
    free(weightlist);
    return(1);
}


// Header: Project name, date, savefile, autosave intervals, source file, writeback file, noisy option, directions of which warnings to not report, etc.
void ReadHeader(struct nnet *net, FILE *input){} //TODO

// Error function, learning algorithm, algorithm parameters or parameters schedule, minibatch size, epoch size, maxEpochs, maxAccuracy, maxDivergence (between
// training & testing accuracy), regularization strategy, regularization parameters, (clipping, L0, L1, L2, L3, Dropout)
void ReadTrainingSection(){} //TODO


// Data section: Specifies inline data, or data locations (files and/or writable pipes).  Training data (readable inputs and outputs) Testing data (readable
// inputs and outputs) Validation data (readable inputs and outputs) Production input (readable inputs) Production output (writable outputs)
void ReadDataSection(){} //TODO


// Node definition statements, until 'EndNodes'
int ReadNodeSection(struct slidingbuffer *bf, struct nnet *net){
    if (bf == NULL || net == NULL) ErrStopParsing(bf,"Program Error: Improper call to ReadNodeSection.",NULL);
    SkipToNext(bf);    if (!AcceptToken(bf, "StartNodes"))return (0);
    if (net->nodecount != 0) ErrStopParsing(bf, "Only one Node Definition section is allowed in a configuration file.",NULL);
    if (ReadCreateNodeStmt(bf,net)) while (ReadCreateNodeStmt(bf, net));
    else ErrStopParsing(bf, "No node definitions found. Expected 'CreateInput','CreateHidden' or 'CreateOutput'.",NULL);
    SkipToNext(bf);    if (!AcceptToken(bf, "EndNodes"))ErrStopParsing(bf,"Expected 'EndNodes' terminator after Node Definition section.",NULL);
    if (net->nodecount == 0) ErrStopParsing(bf, "\nNo nodes created.",NULL);
    return (1);
}

// Check at end of connections and add warning messages for questionable topology.    TODO: suppress these warnings/errors with a header statement.
int ValidateConnections(struct slidingbuffer *bf, struct nnet *net){
    if (bf == NULL || net == NULL) ErrStopParsing(bf, "Program Error: Improper call to ValidateConnections.", NULL);
    int indexconn = 0;    int indexnode = 1;  char wstr[WARNSIZE];
    while (net->sources[indexconn] != 0 && indexconn++ < net->synapsecount);
    if (indexconn == net->synapsecount)
	AddWarning(bf, "Warning:  Are you sure you wanted to define a network with no bias connections (connections with source 0)?");
    for (indexnode = 1; indexnode < net->nodecount - net->outputcount; indexnode++){
	for (indexconn = 0; indexconn < net->synapsecount && net->sources[indexconn] != indexnode; indexconn++);
	if (indexconn == net->synapsecount){
	    if (indexnode <= net->inputcount)snprintf(wstr, WARNSIZE, "Warning: node %d is an input node but does not send any signals.", indexnode);
	    else snprintf(wstr,WARNSIZE, "Warning: node %d is a hidden node that does not send any signals.",indexnode);
	    AddWarning(bf, wstr);}
    }
    for (indexnode = net->inputcount+1; indexnode < net->nodecount; indexnode++){
	for (indexconn = 0; indexconn < net->synapsecount && net->dests[indexconn] != indexnode; indexconn++);
	if (indexconn == net->synapsecount){
	    if (indexnode >= net->nodecount - net->outputcount)
		snprintf(wstr,WARNSIZE,"Warning: node %d is an output node but does not receive any signals.", indexnode);
	    else snprintf(wstr,WARNSIZE,"Warning: node %d is a hidden node that does not receive any signals.",indexnode);
	    AddWarning(bf,wstr);}
    }
    return (1); // return value signals parse failure if 0. For now always returns 1.  TODO, optionally trigger topological reduction with it.
}


// Connection statements, until 'EndConnections'
int ReadConnections(struct slidingbuffer *bf, struct nnet *net){
    assert(bf != NULL); assert (net != NULL);
    SkipToNext(bf);
    if (!AcceptToken(bf, "StartConnections")) return (0);
    if (net->nodecount == 0) ErrStopParsing(bf, "Found 'StartConnections', expected 'StartNodes.' Nodes cannot be connected before they are defined.",NULL);
    SkipToNext(bf);
    if (ReadConnectStmt(bf,net))while (ReadConnectStmt(bf, net));else ErrStopParsing(bf, "No 'Connect' statement found.",NULL);
    SkipToNext(bf);
    if (!AcceptToken(bf, "EndConnections")) ErrStopParsing(bf,"Expected 'Connect' statement or 'EndConnections' terminator.",NULL);
    if (net->synapsecount == 0) ErrStopParsing(bf, "No connections created.",NULL);
    return ValidateConnections(bf,net);
}

// prints low-level information about network suitable for automated analysis via sed, etc.
void debugnnet(struct nnet *net){
    assert(net != NULL);
    int count;
    printf("nodecount %d, inputcount %d, outputcount %d\n", net->nodecount, net->inputcount, net->outputcount);
    printf("Nodes\n");
    for (count = 0; count < net->nodecount; count++){
	if (count == 0) printf("   Bias Node 0: Always outputs one.\n");
	else if (count < net->inputcount) printf("   Input ");
	else if (count < net->nodecount-net->outputcount) printf("   Hidden ");
	else printf("   Output ");
	if (count != 0)
	    printf("Node %d: accum %s, transfer %s, width %d\n", count, acctokens[net->accum[count]], outtokens[net->transfer[count]], net->transferwidths[count]);
    }
    printf("Connections\n");
    for (count = 0; count < net->synapsecount; count++)
	printf("#%d:%d(x%.3f)->%d  ", count, net->sources[count], net->weights[count],net->dests[count]);
    printf("\n");
}


void nnetparser(struct nnet *net, struct slidingbuffer *input){
    assert(net != NULL); assert(input != NULL);
    //    ReadHeader(net,conf);
    //    ReadTrainingSection(net,conf);
    //    ReadDataSection(net,conf);
    SkipToNext(input);
    if (!TokenAvailable(input, "StartNodes") && !TokenAvailable(input, "StartConnections"))
	ErrStopParsing(input, "Expected 'StartNodes'  or 'StartConnections' statement.",NULL);
    SkipToNext(input);
    while (TokenAvailable(input, "StartNodes") || TokenAvailable(input, "StartConnections"))
	if (!ReadNodeSection(input, net) && !ReadConnections(input,net))  ErrStopParsing(input, "Program Error in routine 'parser'. ",NULL);
	else SkipToNext(input);
}
