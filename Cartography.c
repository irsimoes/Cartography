/*
 largura maxima = 100 colunas
 tab = 4 espaços
 0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789

 Linguagens e Ambientes de Programacao (B) - Projeto de 2019/20

 Cartography.c

 Este ficheiro constitui apenas um ponto de partida para o
 seu trabalho. Todo este ficheiro pode e deve ser alterado
 a vontade, a comecar por este comentario. E preciso inventar
 muitas funcoes novas.

 COMPILACAO

 gcc -std=c11 -o Main Cartography.c Main.c -lm

 IDENTIFICACAO DOS AUTORES

 Aluno 1: 55261 Rodrigo Felix
 Aluno 2: 55584 Ines Simoes

 COMENTARIO

 Neste projeto optamos por criar os vetores dinamicamente, para nao sobrecarregar o stack, 
 alocando assim espaço para os vetores no heap, e libertando posteriormente o espaco 
 alocado

 */
#define USE_PTS        true
#include "Cartography.h"

/* STRING -------------------------------------- */

static void showStringVector(StringVector sv, int n) {
	int i;
	for (i = 0; i < n; i++) {
		printf("%s\n", sv[i]);
	}
}

/* UTIL */

static void error(String message) {
	fprintf(stderr, "%s.\n", message);
	exit(1);	// Termina imediatamente a execução do programa
}

static void readLine(String line, FILE *f)// lê uma linha que existe obrigatoriamente
{
	if (fgets(line, MAX_STRING, f) == NULL)
		error("Ficheiro invalido");
	line[strlen(line) - 1] = '\0';	// elimina o '\n'
}

static int readInt(FILE *f) {
	int i;
	String line;
	readLine(line, f);
	sscanf(line, "%d", &i);
	return i;
}

/* IDENTIFICATION -------------------------------------- */

static Identification readIdentification(FILE *f) {
	Identification id;
	String line;
	readLine(line, f);
	sscanf(line, "%s %s %s", id.freguesia, id.concelho, id.distrito);
	return id;
}

static void showIdentification(int pos, Identification id, int z) {
	if (pos >= 0) // pas zero interpretado como não mostrar
		printf("%4d ", pos);
	else
		printf("%4s ", "");
	if (z == 3)
		printf("%-25s %-13s %-22s", id.freguesia, id.concelho, id.distrito);
	else if (z == 2)
		printf("%-25s %-13s %-22s", "", id.concelho, id.distrito);
	else
		printf("%-25s %-13s %-22s", "", "", id.distrito);
}

static void showValue(int value) {
	if (value < 0) // value negativo interpretado como char
		printf(" [%c]\n", -value);
	else
		printf(" [%3d]\n", value);
}

static bool sameIdentification(Identification id1, Identification id2, int z) {
	if (z == 3)
		return strcmp(id1.freguesia, id2.freguesia) == 0
				&& strcmp(id1.concelho, id2.concelho) == 0
				&& strcmp(id1.distrito, id2.distrito) == 0;
	else if (z == 2)
		return strcmp(id1.concelho, id2.concelho) == 0
				&& strcmp(id1.distrito, id2.distrito) == 0;
	else
		return strcmp(id1.distrito, id2.distrito) == 0;
}

/* COORDINATES -------------------------------------- */

Coordinates coord(double lat, double lon) {
	Coordinates c = { lat, lon };
	return c;
}

static Coordinates readCoordinates(FILE *f) {
	double lat, lon;
	String line;
	readLine(line, f);
	sscanf(line, "%lf %lf", &lat, &lon);
	return coord(lat, lon);
}

bool sameCoordinates(Coordinates c1, Coordinates c2) {
	return c1.lat == c2.lat && c1.lon == c2.lon;
}

static double toRadians(double deg) {
	return deg * PI / 180.0;
}

// https://en.wikipedia.org/wiki/Haversine_formula
double haversine(Coordinates c1, Coordinates c2) {
	double dLat = toRadians(c2.lat - c1.lat);
	double dLon = toRadians(c2.lon - c1.lon);

	double sa = sin(dLat / 2.0);
	double so = sin(dLon / 2.0);

	double a = sa * sa
			+ so * so * cos(toRadians(c1.lat)) * cos(toRadians(c2.lat));
	return EARTH_RADIUS * (2 * asin(sqrt(a)));
}

/* RECTANGLE -------------------------------------- */

Rectangle rect(Coordinates tl, Coordinates br) {
	Rectangle r = { tl, br };
	return r;
}

static void showRectangle(Rectangle r) {
	printf("{%lf, %lf, %lf, %lf}", r.topLeft.lat, r.topLeft.lon,
			r.bottomRight.lat, r.bottomRight.lon);
}

static Rectangle calculateBoundingBox(Coordinates vs[], int n) {
////// FAZER
	if (n == 0) {
		return rect(coord(-9999, -9999), coord(9999, 9999));
	}

	Coordinates topleft = coord(-90, 180);
	Coordinates bottomright = coord(90, -180);

	for (int i = 0; i < n; i++) {
		if (vs[i].lat >= topleft.lat) {
			topleft.lat = vs[i].lat;
		}

		if (vs[i].lon <= topleft.lon) {
			topleft.lon = vs[i].lon;
		}

		if (vs[i].lat <= bottomright.lat) {
			bottomright.lat = vs[i].lat;
		}

		if (vs[i].lon >= bottomright.lon) {
			bottomright.lon = vs[i].lon;
		}
	}
	return rect(topleft, bottomright);
}

bool insideRectangle(Coordinates c, Rectangle r) {
	////// FAZER
	double maxlat = r.topLeft.lat;
	double minlon = r.topLeft.lon;
	double minlat = r.bottomRight.lat;
	double maxlon = r.bottomRight.lon;
	return (c.lat <= maxlat && c.lat >= minlat && c.lon <= maxlon
			&& c.lon >= minlon);
}

/* RING -------------------------------------- */

static Ring readRing(FILE *f) {
	Ring r;
	int i, n = readInt(f);
#if USE_PTS
	r.vertexes = malloc(sizeof(Coordinates) * n);
#else
	if (n > MAX_VERTEXES)
			error("Anel demasiado extenso");
#endif
	r.nVertexes = n;
	for (i = 0; i < n; i++) {
		r.vertexes[i] = readCoordinates(f);
	}
	r.boundingBox = calculateBoundingBox(r.vertexes, r.nVertexes);
	return r;
}

// http://alienryderflex.com/polygon/
bool insideRing(Coordinates c, Ring r) {
	if (!insideRectangle(c, r.boundingBox))	// otimização
		return false;
	double x = c.lon, y = c.lat;
	int i, j;
	bool oddNodes = false;
	for (i = 0, j = r.nVertexes - 1; i < r.nVertexes; j = i++) {
		double xi = r.vertexes[i].lon, yi = r.vertexes[i].lat;
		double xj = r.vertexes[j].lon, yj = r.vertexes[j].lat;
		if (((yi < y && y <= yj) || (yj < y && y <= yi))
				&& (xi <= x || xj <= x)) {
			oddNodes ^= (xi + (y - yi) / (yj - yi) * (xj - xi)) < x;
		}
	}
	return oddNodes;
}

bool adjacentRings(Ring a, Ring b) {
////// FAZER
	int i, j;
	for (i = 0; i < a.nVertexes; i++) {
		for (j = 0; j < b.nVertexes; j++) {
			if (sameCoordinates(a.vertexes[i], b.vertexes[j])) {
				return true;
			}
		}
	}
	return false;
}

/* PARCEL -------------------------------------- */

static Parcel readParcel(FILE *f) {
	Parcel p;
	p.identification = readIdentification(f);
	int i, n = readInt(f);
#if USE_PTS
	p.holes = malloc(sizeof(Ring) * n);
#else
	if (n > MAX_HOLES)
			error("Poligono com demasiados buracos");
#endif

	p.edge = readRing(f);
	p.nHoles = n;
	for (i = 0; i < n; i++) {
		p.holes[i] = readRing(f);
	}
	return p;
}

static void showHeader(Identification id) {
	showIdentification(-1, id, 3);
	printf("\n");
}

static void showParcel(int pos, Parcel p, int lenght) {
	showIdentification(pos, p.identification, 3);
	showValue(lenght);
}

bool insideParcel(Coordinates c, Parcel p) {
////// FAZER
	for (int i = 0; i < p.nHoles; i++) {
		if (insideRing(c, p.holes[i])) {
			return false;
		}
	}

	if (insideRing(c, p.edge)) {
		return true;
	}

	return false;
}

//FAZER
bool minBoundOverlap(Rectangle a, Rectangle b) {

	double topA = a.topLeft.lat, botA = a.bottomRight.lat,
			leftA = a.topLeft.lon, rightA = a.bottomRight.lon;
	double topB = b.topLeft.lat, botB = b.bottomRight.lat,
			leftB = b.topLeft.lon, rightB = b.bottomRight.lon;

	if (insideRectangle(a.bottomRight, b) || insideRectangle(b.bottomRight, a)
			|| insideRectangle(a.topLeft, b) || insideRectangle(b.topLeft, a)
			|| insideRectangle(coord(botA, leftA), b)
			|| insideRectangle(coord(botB, leftB), a)
			|| insideRectangle(coord(topA, rightA), b)
			|| insideRectangle(coord(topB, rightB), a)) {

		return true;
	}

	//embora em principio nunca aconteca
	if (((topA > topB) && (botA < botB) && (leftA > leftB) && (rightA < rightB))
			|| ((topB > topA) && (botB < botA) && (leftB > leftA)
					&& (rightB < rightA))) {
		return true;
	}
	return false;
}

bool adjacentParcels(Parcel a, Parcel b) {

	if (!minBoundOverlap(
			calculateBoundingBox(a.edge.vertexes, a.edge.nVertexes),
			calculateBoundingBox(b.edge.vertexes, b.edge.nVertexes))) {
		return false;
	}

	if (adjacentRings(a.edge, b.edge)) {
		return true;
	}

	for (int i = 0; i < b.nHoles; i++) {
		if (adjacentRings(a.edge, b.holes[i])) {
			return true;
		}
	}

	for (int i = 0; i < a.nHoles; i++) {
		if (adjacentRings(b.edge, a.holes[i])) {
			return true;
		}
	}

	return false;
}

/* CARTOGRAPHY -------------------------------------- */

int loadCartography(String fileName, Cartography *cartography) {
	FILE *f;
	int i;
	f = fopen(fileName, "r");
	if (f == NULL)
		error("Impossivel abrir ficheiro");
	int n = readInt(f);

#if USE_PTS
	*cartography = malloc(sizeof(Parcel) * n);
#else
	if (n > MAX_PARCELS)
	error("Demasiadas parcelas no ficheiro");
#endif
	for (i = 0; i < n; i++) {
		(*cartography)[i] = readParcel(f);
	}
	fclose(f);
	return n;
}

static int findLast(Cartography cartography, int n, int j, Identification id) {
	for (; j < n; j++) {
		if (!sameIdentification(cartography[j].identification, id, 3))
			return j - 1;
	}
	return n - 1;
}

void showCartography(Cartography cartography, int n) {
	int last;
	Identification header = { "___FREGUESIA___", "___CONCELHO___",
			"___DISTRITO___" };
	showHeader(header);
	for (int i = 0; i < n; i = last + 1) {
		last = findLast(cartography, n, i, cartography[i].identification);
		showParcel(i, cartography[i], last - i + 1);
	}
}

/* INTERPRETER -------------------------------------- */

static bool checkArgs(int arg) {
	if (arg != -1)
		return true;
	else {
		printf("ERRO: FALTAM ARGUMENTOS!\n");
		return false;
	}
}

static bool checkPos(int pos, int n) {
	if (0 <= pos && pos < n)
		return true;
	else {
		printf("ERRO: POSICAO INEXISTENTE!\n");
		return false;
	}
}

// L
static void commandListCartography(Cartography cartography, int n) {
	showCartography(cartography, n);
}

// M pos

int totalVertexes(Parcel parcel) {
	int sum = parcel.edge.nVertexes;
	if (parcel.nHoles != 0) {
		for (int i = 0; i < parcel.nHoles; i++) {
			sum += parcel.holes[i].nVertexes;
		}
	}
	return sum;
}

static void commandMaximum(int pos, Cartography cartography, int n) {
	if (!checkArgs(pos) || !checkPos(pos, n))
		return;
	////// FAZER

	int nVertexes = -1;
	int max = pos;

	Identification id = cartography[pos].identification;

	for (int i = 1;
			pos - i >= 0
					&& sameIdentification(id,
							cartography[pos - i].identification, 3); i++) {
		if (totalVertexes(cartography[pos - i]) > nVertexes) {
			max = pos - i;
			nVertexes = totalVertexes(cartography[pos - i]);
		}
	}

	for (int i = 0;
			sameIdentification(id, cartography[pos + i].identification, 3);
			i++) {
		if (totalVertexes(cartography[pos + i]) > nVertexes) {
			max = pos + i;
			nVertexes = totalVertexes(cartography[pos + i]);
		}
	}

	showParcel(max, cartography[max], nVertexes);
}

// X
static void extremes(Cartography cartography, int n) {

	int north = 0, south = 0, east = 0, west = 0;
	double maxLat = -90, minLat = 90, maxLon = -180, minLon = 180;

	for (int i = 0; i < n; i++) {
		if (cartography[i].edge.boundingBox.topLeft.lat >= maxLat) {
			maxLat = cartography[i].edge.boundingBox.topLeft.lat;
			north = i;
		}

		if (cartography[i].edge.boundingBox.bottomRight.lat <= minLat) {
			minLat = cartography[i].edge.boundingBox.bottomRight.lat;
			south = i;
		}

		if (cartography[i].edge.boundingBox.bottomRight.lon >= maxLon) {
			maxLon = cartography[i].edge.boundingBox.bottomRight.lon;
			east = i;
		}

		if (cartography[i].edge.boundingBox.topLeft.lon <= minLon) {
			minLon = cartography[i].edge.boundingBox.topLeft.lon;
			west = i;
		}
	}

	showParcel(north, cartography[north], -'N');
	showParcel(east, cartography[east], -'E');
	showParcel(south, cartography[south], -'S');
	showParcel(west, cartography[west], -'W');

}

//R pos

void summary(int pos, Cartography cartography, int n) {

	if (!checkArgs(pos) || !checkPos(pos, n))
		return;

	showIdentification(pos, cartography[pos].identification, 3);
	printf("\n     %d ", cartography[pos].edge.nVertexes);

	for (int i = 0; i < cartography[pos].nHoles; i++) {
		printf("%d ", cartography[pos].holes[i].nVertexes);
	}

	showRectangle(cartography[pos].edge.boundingBox);
	printf("\n");
}

//V lat lon pos
void trip(double lat, double lon, int pos, Cartography cartography, int n) {

	if (!checkArgs(pos) || !checkPos(pos, n))
		return;

	Coordinates coordinates = coord(lat, lon);
	double minDist = haversine(cartography[pos].edge.vertexes[0], coordinates);

	for (int i = 1; i < cartography[pos].edge.nVertexes; i++) {
		if (haversine(cartography[pos].edge.vertexes[i], coordinates)
				< minDist) {
			minDist = haversine(cartography[pos].edge.vertexes[i], coordinates);
		}
	}

	printf(" %f\n", minDist);
}

//Q pos

static void showQuantity(int pos, Parcel p, int lenght, int specifics) {
	showIdentification(pos, p.identification, specifics);
	showValue(lenght);
}

void howMany(int pos, Cartography cartography, int n) {

	if (!checkArgs(pos) || !checkPos(pos, n))
		return;

	int pFreg = 0, pConc = 0, pDistr = 0;
	Identification id = cartography[pos].identification;

	for (int i = 0; i < n; i++) {

		if (sameIdentification(id, cartography[i].identification, 3)) {
			pDistr++;
			pConc++;
			pFreg++;

		} else if (sameIdentification(id, cartography[i].identification, 2)) {
			pDistr++;
			pConc++;

		} else if (sameIdentification(id, cartography[i].identification, 1)) {
			pDistr++;
		}

	}

	showQuantity(pos, cartography[pos], pFreg, 3);
	showQuantity(pos, cartography[pos], pConc, 2);
	showQuantity(pos, cartography[pos], pDistr, 1);

}

//C

static int compareString(const void *s1, const void *s2) {
	return strcmp((const char*) s1, (const char*) s2);
}

void todosConcelhos(Cartography cartography, int n) {

	StringVector concelhos;
	int pos = 0;

	strcpy(concelhos[pos++], cartography[0].identification.concelho);

	for (int i = 0; i < n - 1; i++) {
		if (!sameIdentification(cartography[i].identification,
				cartography[i + 1].identification, 2)) {
			strcpy(concelhos[pos++],
					cartography[i + 1].identification.concelho);
		}
	}

	qsort(concelhos, pos, sizeof(String), compareString);
	showStringVector(concelhos, pos);

}

//D
void todosDistritos(Cartography cartography, int n) {

	StringVector distritos;
	int pos = 0;

	strcpy(distritos[pos++], cartography[0].identification.distrito);

	for (int i = 0; i < n - 1; i++) {
		if (!sameIdentification(cartography[i].identification,
				cartography[i + 1].identification, 1)) {
			strcpy(distritos[pos++],
					cartography[i + 1].identification.distrito);
		}
	}

	qsort(distritos, pos, sizeof(String), compareString);
	showStringVector(distritos, pos);

}

//P lat lon
void parcel(double lat, double lon, Cartography cartography, int n) {
	int i;
	bool found = false;
	Coordinates coordinates = coord(lat, lon);

	for (i = 0; i < n && !found; i++) {
		if (insideParcel(coordinates, cartography[i])) {
			showIdentification(i, cartography[i].identification, 3);
			printf("\n");
			found = true;
		}
	}

	if (!found) {
		printf("%s\n", "FORA DO MAPA");
	}

}

//A pos

void adjacency(int pos, Cartography cartography, int n) {

	if (!checkArgs(pos) || !checkPos(pos, n))
		return;

	bool found = false;

	for (int i = 0; i < n; i++) {
		if (adjacentParcels(cartography[pos], cartography[i]) && i != pos) {
			found = true;
			showIdentification(i, cartography[i].identification, 3);
			printf("\n");
		}
	}

	if (!found) {
		printf("%s\n", "NAO HA ADJACENCIAS");
	}
}

//F pos1 pos2

static bool adjacencyToArray(int pos, Cartography cartography, int n, int *v,

int *counter, bool *bitmap) {
	bool toret = false;
	for (int i = 0; i < n; i++) {
		if (adjacentParcels(cartography[pos], cartography[i]) && i != pos
				&& !bitmap[i]) {
			toret = true;
			v[*counter] = i;
			*counter += 1;
			bitmap[i] = true;
		}
	}
	return toret;
}
void borders(int pos1, int pos2, Cartography cartography, int n) {

	if (!checkArgs(pos2) || !checkPos(pos1, n) || !checkPos(pos2, n))
		return;

	if (pos1 == pos2) {
		printf("%d\n", 0);
	} else {
		bool pathExists = false;
		int ret = 0;
		int counter = 0;
		bool *bitmap = malloc(sizeof(bool) * n);
		int *v = malloc(sizeof(int) * n);
		// bool bitmap[n];
		// int v[n];
		for (int i = 0; i < n; i++)
			bitmap[i] = false;
		bitmap[pos1] = true;
		if (adjacencyToArray(pos1, cartography, n, v, &counter, bitmap)) {
			ret++;
			int newwave = counter;
			for (int i = 0; i < n; i++) {
				if (v[i] == pos2) {
					printf(" %d\n", ret);
					pathExists = true;
					break;
				}
				adjacencyToArray(v[i], cartography, n, v, &counter, bitmap);
				if (i + 1 == newwave) {
					ret++;
					if (i == counter - 1)
						break;
					newwave = counter;
				}
			}
		}
		if (!pathExists)printf("%s", "NAO HA CAMINHO\n");

		free(bitmap);
		free(v);
	}
}

//T dist
static int compareint(const void *i1, const void *i2) {
	return (*(const int*) i1 - *(const int*) i2);
}

void partitions(double dist, Cartography cartography, int n) {
	bool *bitmap = malloc(sizeof(bool) * n);
	//bool bitmap[n];
	int totalcounter = 0;
	for (int i = 0; i < n; i++)
		bitmap[i] = false;
	int *parts = malloc(sizeof(int) * n);
	//int parts[n];
	parts[0] = 0;
	bitmap[0] = true;
	int counter = 1;
	for (;;) {
		for (int i = 0; i < counter && counter != n; i++) {
			for (int j = 0; j < n; j++) {
				if (!bitmap[j]
						&& haversine(cartography[j].edge.vertexes[0],
								cartography[parts[i]].edge.vertexes[0])
								<= dist) {
					parts[counter++] = j;
					bitmap[j] = true;
				}
			}
		}
		qsort(parts, counter, sizeof(int), compareint);

		int i;
		for (i = 0; i < counter; i++) {
			printf(" %d", parts[i]);
			if (i+1 < counter && parts[i + 1] - parts[i] == 1) {
				for (; i+1 < counter && parts[i + 1] - parts[i] == 1; i++)
					;
				printf("-%d", parts[i]);
			}
		}
		printf("%s", "\n");

		totalcounter += counter;
		if (totalcounter == n)
			break;
		//parts = (int *)realloc(parts, sizeof(int)*(n-totalcounter));
		int pos;
		for (pos = 0; pos < n; pos++) {
			if (bitmap[pos] == false)
				break;
		}
		parts[0] = pos;
		bitmap[pos] = true;
		counter = 1;
	}
	free(parts);
	free(bitmap);
}

void interpreter(Cartography cartography, int n) {
	String commandLine;
	for (;;) {	// ciclo infinito
		printf("> ");
		readLine(commandLine, stdin);
		char command = ' ';
		double arg1 = -1.0, arg2 = -1.0, arg3 = -1.0;
		sscanf(commandLine, "%c %lf %lf %lf", &command, &arg1, &arg2, &arg3);
		// printf("%c %lf %lf %lf\n", command, arg1, arg2, arg3);
		switch (commandLine[0]) {
		case 'L':
		case 'l':	// listar
			commandListCartography(cartography, n);
			break;

		case 'M':
		case 'm':	// maximo
			commandMaximum(arg1, cartography, n);
			break;

		case 'X':
		case 'x': //extremos
			extremes(cartography, n);
			break;

		case 'R':
		case 'r': //resumo
			summary(arg1, cartography, n);
			break;

		case 'V':
		case 'v': //viagem
			trip(arg1, arg2, arg3, cartography, n);
			break;

		case 'Q':
		case 'q': //quantos
			howMany(arg1, cartography, n);
			break;

		case 'C':
		case 'c': //concelhos
			todosConcelhos(cartography, n);
			break;

		case 'D':
		case 'd': //distritos
			todosDistritos(cartography, n);
			break;

		case 'P':
		case 'p': //parcela
			parcel(arg1, arg2, cartography, n);
			break;

		case 'A':
		case 'a': //adjacencias
			adjacency(arg1, cartography, n);
			break;

		case 'F':
		case 'f': //fronteiras
			borders(arg1, arg2, cartography, n);
			break;

		case 'T':
		case 't': //particao
			partitions(arg1, cartography, n);
			break;

		case 'Z':
		case 'z':	// terminar
			printf("Fim de execucao! Volte sempre.\n");
			for (int i = 0; i < n; i++) {
				for (int j = 0; j < cartography[i].nHoles; j++) {
					free(cartography[i].holes[j].vertexes);
				}
				free(cartography[i].edge.vertexes);
				free(cartography[i].holes);
			}
			free(cartography);
			return;

		default:
			printf("Comando desconhecido: \"%s\"\n", commandLine);
		}
	}
}