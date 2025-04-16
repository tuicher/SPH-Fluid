// PBF_System.cpp
#include "PBF_System.h"

Scalar PBF_System::CalcDensity(const int target_index)
{
    const auto& p_target = particles[target_index];

    Scalar density = 0.0;
    for (int neighbor_index : neighborSearchEngine.retrieveNeighbors(target_index))
    {
        const auto& p = particles[neighbor_index];
        density += p.m * CalcKernel(p_target.p - p.p, radius);
    }

    return density;
}

Scalar PBF_System::CalcConstraint(const int target_index)
{
    const Scalar density = CalcDensity(target_index);
    return (density / restDensity) - 1.0;
}

Vec3 PBF_System::CalcGradConstraint(const int target_index, const int var_index)
{
    const PBF_Particle& p_target = particles[target_index];

    if (target_index == var_index)
    {
        Vec3 sum = Vec3::Zero();

        for (int neighbor_index : neighborSearchEngine.retrieveNeighbors(target_index))
        {
            const auto& p = particles[neighbor_index];

            sum += p.m * CalcGradKernel(p_target.p - p.p, radius);
        }

        return sum / restDensity;
    }
    else
    {
        const auto& p = particles[var_index];

        return -p.m * CalcGradKernel(p_target.p - p.p, radius) / restDensity;
    }
}

void PBF_System::PrintAverageNumNeighbors()
{
    const int num_particles = neighborSearchEngine.getNumParticles();

    VecX nums(num_particles);
    for (int i = 0; i < num_particles; ++i)
    {
        nums[i] = neighborSearchEngine.retrieveNeighbors(i).size();
    }

    printf("Average(#neighbors): %d\n",nums.mean());
}

void PBF_System::PrintAverageDensity()
{
    VecX buffer(particles.size());
    for (int i = 0; i < particles.size(); ++i)
    {
        buffer[i] = CalcDensity(i);
    }

    printf("Average(density): %d\n", buffer.mean());
}

void PBF_System::SetParticlesColors()
{
    // Calcular rango de Y para el gradiente
    Scalar minY = std::numeric_limits<Scalar>::max();
    Scalar maxY = std::numeric_limits<Scalar>::lowest();
    for (const auto& p : particles)
    {
        Scalar y = p.x.y();
        if (y < minY) minY = y;
        if (y > maxY) maxY = y;
    }

    // Evitar división por cero
    if (maxY == minY) maxY += 1.0;

    // Asignar colores en arcoíris según Y
    for (auto& p : particles)
    {
        // Normalizar Y entre 0 y 1 (invertido para arriba-abajo)
        Scalar t = (maxY - p.x.y()) / (maxY - minY);

        // Convertir a HSV (Hue: 0°=rojo, 300°=magenta)
        Scalar hue = t * 300.0;
        Scalar s = 1.0, v = 1.0;
        Scalar c = v * s;
        Scalar h_prime = hue / 60.0;
        Scalar x = c * (1 - std::abs(std::fmod(h_prime, 2.0) - 1.0));
        Scalar m = v - c;

        int i = static_cast<int>(h_prime) % 6;
        Scalar r, g, b;

        switch (i) {
        case 0: r = c; g = x; b = 0; break;
        case 1: r = x; g = c; b = 0; break;
        case 2: r = 0; g = c; b = x; break;
        case 3: r = 0; g = x; b = c; break;
        case 4: r = x; g = 0; b = c; break;
        case 5: r = c; g = 0; b = x; break;
        default: r = g = b = 0; break;
        }

        // Ajustar brillo y asegurar rango [0,1]
        p.color = Eigen::Vector3f(
            std::clamp(r + m, 0.0, 1.0),
            std::clamp(g + m, 0.0, 1.0),
            std::clamp(b + m, 0.0, 1.0)
        );
    }
}

void PBF_System::InitSystem()
{
    printf("#### PBF System ####\n");
    printf("Particles: %d\n", numParticles);
    printf("TimeStep: %f\n", timeStep);
    printf("SubSteps: %d\n", numSubSteps);

    particles.resize(numParticles);
    for (int i = 0; i < numParticles; ++i)
    {
        particles[i].i = i;
        particles[i].m = 3000.0 / static_cast<Scalar>(numParticles);
        /*
        // Generar posiciones dentro de una esfera de radio centrada en (0, 4, 0)
        Vec3 spherePos = Vec3::Random().normalized();
        spherePos *= 0.75; // Escalar al radio deseado
        particles[i].x = spherePos + Vec3(0.0, 4.0, 0.0);
        */
        // Generar posiciones en un paralelepipedo
        particles[i].x = Vec3(0.5, 2.0, 0.5).cwiseProduct(Vec3::Random()) + Vec3(0.0, 4.0, 0.0);
        particles[i].v = Vec3::Zero();
    }

    // Relax initial particle positions (a dirty solution for resolving bad initial states)
    constexpr int num_relax_steps = 20;
    for (int k = 0; k < num_relax_steps; ++k)
    {
        const Scalar damping = std::min(1.0, (static_cast<Scalar>(k) * 2.0 / static_cast<Scalar>(num_relax_steps)));

        // Step the simulation time forward using a very small time step
        Step(1e-04 * timeStep);

        // Damp velocities for stability
        for (auto& p : particles)
        {
            p.v = damping * p.v;
        }
    }

    for (auto& p : particles)
    {
        p.v = Vec3::Zero();
        //p.v = Vec3(15.0, 0.0, 0.0);
    }

    
}



PBF_System::PBF_System() : neighborSearchEngine(radius, particles)
{
    // Init Particles
    InitSystem();

    SetParticlesColors();
}

PBF_System::~PBF_System() 
{
	//free(particles);
}

void PBF_System::AnimationStep()
{
    const Scalar sub_dt = timeStep / static_cast<Scalar>(numSubSteps);

    for (int k = 0; k < numSubSteps; ++k)
    {
        Step(sub_dt);
    }
}

void PBF_System::Step(const Scalar dt)
{
    //printf("Number of particles %d\n", particles.size());

    // Predict positions using the semi-implicit Euler integration
    #pragma omp parallel for
    for (int i = 0; i < numParticles; ++i)
    {
        particles[i].v = particles[i].v + dt * Vec3(0.0, -9.8, 0.0);
        particles[i].p = particles[i].x + dt * particles[i].v;
    }

    // Perform neighbor search based on updated positions
    neighborSearchEngine.searchNeighbors();

    if (verbose)
    {
        PrintAverageNumNeighbors();
        PrintAverageDensity();
    }

    const int num_iters = 2;
    #pragma omp parallel for
    for (int k = 0; k < num_iters; ++k)
    {
        VecX lambda(numParticles);
        for (int i = 0; i < numParticles; ++i)
        {
            const PBF_Particle& p = particles[i];
            const Scalar numerator = CalcConstraint(i);

            Scalar denominator = 0.0;

            #pragma omp parallel for
            for (int neighbor_index : neighborSearchEngine.retrieveNeighbors(i))
            {
                const Vec3 grad = CalcGradConstraint(i, neighbor_index);

                // Note: In Eq.12, the inverse mass is dropped for simplicity
                denominator += (1.0 / particles[neighbor_index].m) * grad.squaredNorm();
            }


            // Note: Add an epsilon value for relaxation (see Eq.11)
            // TODO: Check this equation
            denominator += epsilon;

            lambda[i] = -numerator / denominator;
        }

        // Calculate delta p in the Jacobi style
        MatX delta_p(3, numParticles);


        #pragma omp parallel for
        for (int i = 0; i < numParticles; ++i)
        {
            const PBF_Particle& p = particles[i];
            const auto& neighbors = neighborSearchEngine.retrieveNeighbors(i);
            const int numNeighbors = neighbors.size();

            // Calculate the artificial tensile pressure correction constant
            constexpr Scalar corr_n = 4.0;
            constexpr Scalar corr_h = 0.30;
            const Scalar corr_k = p.m * 1.0e-04; // Note: This equation has no ground and may not work well
            const Scalar corr_w = CalcKernel(corr_h * radius * Vec3::UnitX(), radius);

            // Calculate the sum of pressure effect (Eq.12)
            MatX buffer(3, numNeighbors);
            #pragma omp parallel for
            for (int j = 0; j < numNeighbors; ++j)
            {
                const int neighborIndex = neighbors[j];

                // Calculate the artificial tensile pressure correction
                const Scalar kernel_val = CalcKernel(p.p - particles[neighborIndex].p, radius);
                const Scalar ratio = kernel_val / corr_w;
                const Scalar corr_coeff = -corr_k * std::pow(ratio, corr_n);

                const Scalar coeff = particles[neighborIndex].m * (lambda[i] + lambda[neighborIndex] + corr_coeff);

                buffer.col(j) = coeff * CalcGradKernel(p.p - particles[neighborIndex].p, radius);
            }
            const Vec3 sum = buffer.rowwise().sum();

            // Calculate delta p of this particle
            delta_p.col(i) = (1.0 / p.m) * (1.0 / restDensity) * sum;
        }

        // Apply delta p in the Jacobi style
        #pragma omp parallel for
        for (int i = 0; i < numParticles; ++i)
        {
            particles[i].p += delta_p.col(i);
        }

        // Solve collision constraints
        #pragma omp parallel for
        for (int i = 0; i < numParticles; ++i)
        {
            auto& p = particles[i];

            // Detect and resolve environmental collisions (in a very naive way)
            p.p = p.p.cwiseMax(Vec3(-30.0, 0.0, -30.0));
            p.p = p.p.cwiseMin(Vec3(+30.0, 8.0, +30.0));
        }
    }
    // Update positions and velocities
    #pragma omp parallel for
    for(int i = 0; i < numParticles; ++i)
    {
        particles[i].v = damping * (particles[i].p - particles[i].x) / dt;
        particles[i].x = particles[i].p;
    }

    // Apply the XSPH viscosity effect [Schechter+, SIGGRAPH 2012]
    VecX densities(numParticles);
    MatX delta_v(3, numParticles);

    // Update positions and velocities
    #pragma omp parallel for
    for (int i = 0; i < numParticles; ++i)
    {
        densities[i] = CalcDensity(i);
    }

    #pragma omp parallel for
    for (int i = 0; i < numParticles; ++i)
    {
        const PBF_Particle& p = particles[i];
        const auto& neighbors = neighborSearchEngine.retrieveNeighbors(i);
        const int numNeighbors = neighbors.size();

        MatX buffer(3, numNeighbors);
        #pragma omp parallel for
        for (int j = 0; j < numNeighbors; ++j)
        {
            const int neighbor_index = neighbors[j];
            const Scalar kernel_val = CalcKernel(p.x - particles[neighbor_index].x, radius);
            const auto rel_velocity = particles[neighbor_index].v - p.v;

            buffer.col(j) = (p.m / densities[neighbor_index]) * kernel_val * rel_velocity;
        }
        const auto sum = buffer.rowwise().sum();

        delta_v.col(i) = viscosity * sum;
    }
    
    #pragma omp parallel for
    for (int i = 0; i < numParticles; ++i)
    {
        particles[i].v += delta_v.col(i);
    }
    // TODO: Apply vorticity confinement
}
