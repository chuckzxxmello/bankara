import { Tile, Grid, Column } from '@carbon/react';
import './Features.css';

const features = [
  {
    title: 'Cryptocurrency',
    description: 'Secure digital asset trading and management.',
  },
  {
    title: 'Digital E-Wallet',
    description: 'Fast, secure payment infrastructure.',
  },
  {
    title: 'Banking',
    description: 'Scalable modern banking solutions.',
  },
];

export function Features() {
  return (
    <section className="features">
      <div className="features-container">
        <h2 className="features-title">Our Services</h2>
        <Grid>
          {features.map((feature) => (
            <Column key={feature.title} sm={4} md={4} lg={4}>
              <Tile className="feature-tile">
                <h3 className="feature-title">{feature.title}</h3>
                <p className="feature-description">{feature.description}</p>
              </Tile>
            </Column>
          ))}
        </Grid>
      </div>
    </section>
  );
}
