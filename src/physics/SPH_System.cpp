#include "SPH_System.h"

SPH_System::SPH_System()
{
	maxParticles = 30000;
	numParticles = 0; 

	kernel = 0.04f;
	mass = 0.02f;

	worldSize = Eigen::Vector3f( 0.64f, 0.64f, 0.64f);

	cellSize = kernel;

	gridSize = Eigen::Vector3i(
		ceil(worldSize.x() / cellSize),
		ceil(worldSize.y() / cellSize),
		ceil(worldSize.z() / cellSize));

	totCell = gridSize.x() * gridSize.y() * gridSize.z();
	
	gravity = Eigen::Vector3f( 0.0f, -6.8f, 0.0f);

	wallDamping = -0.5f;
	restDensity = 1000.0f;
	gasConstant = 1.0f;
	viscosity = 6.5f;
	timeStep = 0.003f;
	surfNorm = 6.0f;
	surfCoe = 0.1f;

	poly6Value = 315.0f / (64.0f * M_PI * pow(kernel, 9));;
	spikyValue = -45.0f / (M_PI * pow(kernel, 6));
	viscoValue = 45.0f / (M_PI * pow(kernel, 6));

	gradPoly6 = -945 / (32 * M_PI * pow(kernel, 9));
	lplcPoly6 = -945 / (8 * M_PI * pow(kernel, 9));

	kernel_2 = kernel * kernel;
	self_dens = mass * poly6Value * pow(kernel, 6);
	self_lplc_color = lplcPoly6 * mass * kernel_2 * (0 - 3 / 4 * kernel_2);

	mem = (Particle*) malloc(sizeof(Particle) * maxParticles);
	cell = (Particle**) malloc(sizeof(Particle*) * totCell);

	sys_running = 0;

	printf("Initialize SPH_System:\n");
	printf("World Width : %f\n", worldSize.x());
	printf("World Height: %f\n", worldSize.y());
	printf("World Length: %f\n", worldSize.z());
	printf("Cell Size  : %f\n", cellSize);
	printf("Grid Width : %u\n", gridSize.x());
	printf("Grid Height: %u\n", gridSize.y());
	printf("Grid Length: %u\n", gridSize.z());
	printf("Total Cell : %u\n", totCell);
	printf("Poly6 Kernel: %f\n", poly6Value);
	printf("Spiky Kernel: %f\n", spikyValue);
	printf("Visco Kernel: %f\n", viscoValue);
	printf("Self Density: %f\n", self_dens);
}

SPH_System::~SPH_System()
{
	free(mem);
	free(cell);
}

// Simulates a Step of simulation
void SPH_System::Animation()
{
	if (sys_running == 0)
	{
		return;
	}

	BuildTable();
	Comp_DensPres();
	Comp_ForceAdv();
	Advection();
}

void SPH_System::InitSystem()
{
	Eigen::Vector3f pos;
	Eigen::Vector3f vel = Eigen::Vector3f( 0.0f, 0.0f, 0.0f);

	for (pos.x() = worldSize.x() * 0.5f; pos.x() < worldSize.x() * 0.9f; pos.x() += (kernel * 0.5f))
	{
		for (pos.y() = worldSize.y() * 0.5f; pos.y() < worldSize.y() * 0.9f; pos.y() += (kernel * 0.5f))
		{
			for (pos.z() = worldSize.z() * 0.5f; pos.z() < worldSize.z() * 0.9f; pos.z() += (kernel * 0.5f))
			{
				AddParticle( pos, vel);
			}
		}
	}

	printf("Init Particle: %u\n", numParticles);
}

void SPH_System::AddParticle(Eigen::Vector3f pos, Eigen::Vector3f vel)
{
	Particle *p = &(mem[numParticles]);

	p->id = numParticles;
	p->pos = pos;
	p->vel = vel;

	p->acc = Eigen::Vector3f(0.0f, 0.0f, 0.0f);
	p->ev = Eigen::Vector3f(0.0f, 0.0f, 0.0f);
	p->color = Eigen::Vector3f(0.0f, 0.0f, 0.0f);

	p->dens = restDensity;
	p->pres = 0.0f;

	p->next = NULL;

	numParticles++;
}

void SPH_System::BuildTable()
{
	Particle* p;
	uint hash;

	for (uint i = 0; i < totCell; i++)
	{
		cell[i] = NULL;
	}

	for (uint i = 0; i < numParticles; i++)
	{
		p = &(mem[i]);
		hash = Calc_CellHash(Calc_CellPos(p->pos));

		if (cell[hash] == NULL)
		{
			p->next = NULL;
			cell[hash] = p;
		}
		else
		{
			p->next = cell[hash];
			cell[hash] = p;
		}
	}
}

void SPH_System::Comp_DensPres()
{
	Particle* p;
	Particle* np;

	Eigen::Vector3i cellPos;
	Eigen::Vector3i nearPos;
	uint hash;

	Eigen::Vector3f relPos;
	float r2;

	for (uint i = 0; i < numParticles; i++)
	{
		p = &(mem[i]);
		cellPos = Calc_CellPos(p->pos);

		p->dens = 0.0f;
		p->pres = 0.0f;

		for (int x = -1; x <= 1; x++)
		{
			for (int y = -1; y <= 1; y++)
			{
				for (int z = -1; z <= 1; z++)
				{
					nearPos.x() = cellPos.x() + x;
					nearPos.y() = cellPos.y() + y;
					nearPos.z() = cellPos.z() + z;
					hash = Calc_CellHash(nearPos);

					if (hash == 0xffffffff)
					{
						continue;
					}

					np = cell[hash];
					while (np != NULL)
					{
						relPos.x() = np->pos.x() - p->pos.x();
						relPos.y() = np->pos.y() - p->pos.y();
						relPos.z() = np->pos.z() - p->pos.z();
						r2 = relPos.x() * relPos.x() + relPos.y() * relPos.y() + relPos.z() * relPos.z();

						if (r2 < INF || r2 >= kernel_2)
						{
							np = np->next;
							continue;
						}

						p->dens = p->dens + mass * poly6Value * pow(kernel_2 - r2, 3);

						np = np->next;
					}
				}
			}
		}

		p->dens = p->dens + self_dens;
		p->pres = (pow(p->dens / restDensity, 7) - 1) * gasConstant;
	}
}

void SPH_System::Comp_ForceAdv()
{
	Particle* p;
	Particle* np;

	Eigen::Vector3i cell_pos;
	Eigen::Vector3i near_pos;
	uint hash;

	Eigen::Vector3f relPos;
	Eigen::Vector3f relVel;

	float r2;
	float r;
	float kernel_r;
	float V;

	float presKernel;
	float viscKernel;
	float tempForce;

	Eigen::Vector3f gradColor;
	float lplcColor;

	for (uint i = 0; i < numParticles; i++)
	{
		p = &(mem[i]);
		cell_pos = Calc_CellPos(p->pos);

		p->acc.x() = 0.0f;
		p->acc.y() = 0.0f;
		p->acc.z() = 0.0f;

		gradColor.x() = 0.0f;
		gradColor.y() = 0.0f;
		gradColor.z() = 0.0f;
		lplcColor = 0.0f;

		for (int x = -1; x <= 1; x++)
		{
			for (int y = -1; y <= 1; y++)
			{
				for (int z = -1; z <= 1; z++)
				{
					near_pos.x() = cell_pos.x() + x;
					near_pos.y() = cell_pos.y() + y;
					near_pos.z() = cell_pos.z() + z;
					hash = Calc_CellHash(near_pos);

					if (hash == 0xffffffff)
					{
						continue;
					}

					np = cell[hash];
					while (np != NULL)
					{
						relPos.x() = p->pos.x() - np->pos.x();
						relPos.y() = p->pos.y() - np->pos.y();
						relPos.z() = p->pos.z() - np->pos.z();
						r2 = relPos.x() * relPos.x() + relPos.y() * relPos.y() + relPos.z() * relPos.z();

						if (r2 < kernel_2 && r2 > INF)
						{
							r = sqrt(r2);
							V = mass / np->dens / 2;
							kernel_r = kernel - r;

							presKernel = spikyValue * kernel_r * kernel_r;
							tempForce = V * (p->pres + np->pres) * presKernel;

							p->acc.x() = p->acc.x() - relPos.x() * tempForce / r;
							p->acc.y() = p->acc.y() - relPos.y() * tempForce / r;
							p->acc.z() = p->acc.z() - relPos.z() * tempForce / r;

							relVel.x() = np->ev.x() - p->ev.x();
							relVel.y() = np->ev.y() - p->ev.y();
							relVel.z() = np->ev.z() - p->ev.z();

							viscKernel = viscoValue * (kernel - r);
							tempForce = V * viscosity * viscKernel;
							p->acc.x() = p->acc.x() + relVel.x() * tempForce;
							p->acc.y() = p->acc.y() + relVel.y() * tempForce;
							p->acc.z() = p->acc.z() + relVel.z() * tempForce;

							float temp = (-1) * gradPoly6 * V * pow(kernel_2 - r2, 2);
							gradColor.x() += temp * relPos.x();
							gradColor.y() += temp * relPos.y();
							gradColor.z() += temp * relPos.z();
							lplcColor += lplcPoly6 * V * (kernel_2 - r2) * (r2 - 3 / 4 * (kernel_2 - r2));
						}

						np = np->next;
					}
				}
			}
		}

		lplcColor += self_lplc_color / p->dens;
		p->surf_norm = sqrt(gradColor.x() * gradColor.x() + gradColor.y() * gradColor.y() + gradColor.z() * gradColor.z());

		if (p->surf_norm > surfNorm)
		{
			p->acc.x() += surfCoe * lplcColor * gradColor.x() / p->surf_norm;
			p->acc.y() += surfCoe * lplcColor * gradColor.y() / p->surf_norm;
			p->acc.z() += surfCoe * lplcColor * gradColor.z() / p->surf_norm;
		}
	}
}

void SPH_System::Advection()
{
	Particle* p;

	for (uint i = 0; i < numParticles; i++)
	{
		p = &(mem[i]);

		p->vel.x() = p->vel.x() + p->acc.x() * timeStep / p->dens + gravity.x() * timeStep;
		p->vel.y() = p->vel.y() + p->acc.y() * timeStep / p->dens + gravity.y() * timeStep;
		p->vel.z() = p->vel.z() + p->acc.z() * timeStep / p->dens + gravity.z() * timeStep;

		p->pos.x() = p->pos.x() + p->vel.x() * timeStep;
		p->pos.y() = p->pos.y() + p->vel.y() * timeStep;
		p->pos.z() = p->pos.z() + p->vel.z() * timeStep;

		if (p->pos.x() >= worldSize.x() - BOUNDARY)
		{
			p->vel.x() = p->vel.x() * wallDamping;
			p->pos.x() = worldSize.x() - BOUNDARY;
		}

		if (p->pos.x() < 0.0f)
		{
			p->vel.x() = p->vel.x() * wallDamping;
			p->pos.x() = 0.0f;
		}

		if (p->pos.y() >= worldSize.y() - BOUNDARY)
		{
			p->vel.y() = p->vel.y() * wallDamping;
			p->pos.y() = worldSize.y() - BOUNDARY;
		}

		if (p->pos.y() < 0.0f)
		{
			p->vel.y() = p->vel.y() * wallDamping;
			p->pos.y() = 0.0f;
		}

		if (p->pos.z() >= worldSize.z() - BOUNDARY)
		{
			p->vel.z() = p->vel.z() * wallDamping;
			p->pos.z() = worldSize.z() - BOUNDARY;
		}

		if (p->pos.z() < 0.0f)
		{
			p->vel.z() = p->vel.z() * wallDamping;
			p->pos.z() = 0.0f;
		}

		p->ev.x() = (p->ev.x() + p->vel.x()) / 2;
		p->ev.y() = (p->ev.y() + p->vel.y()) / 2;
		p->ev.z() = (p->ev.z() + p->vel.z()) / 2;
	}
}

Eigen::Vector3i SPH_System::Calc_CellPos(Eigen::Vector3f p)
{
	Eigen::Vector3i cellPos = Eigen::Vector3i(
		int(floor((p.x()) / cellSize)),
		int(floor((p.y()) / cellSize)),
		int(floor((p.z()) / cellSize))
	);

	return cellPos;
}

uint SPH_System::Calc_CellHash(Eigen::Vector3i cellPos)
{
	if (cellPos.x() < 0 || cellPos.x() >= (int)gridSize.x() || cellPos.y() < 0 || cellPos.y() >= (int)gridSize.y() || cellPos.z() < 0 || cellPos.z() >= (int)gridSize.z())
	{
		return (uint)0xffffffff;
	}

	cellPos.x() = cellPos.x() & (gridSize.x() - 1);
	cellPos.y() = cellPos.y() & (gridSize.y() - 1);
	cellPos.z() = cellPos.z() & (gridSize.z() - 1);

	return ((uint)(cellPos.z())) * gridSize.y() * gridSize.x() + ((uint)(cellPos.y())) * gridSize.x() + (uint)(cellPos.x());

}