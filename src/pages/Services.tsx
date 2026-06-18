import { useEffect } from 'react';
import { Grid, Column, Tile } from '@carbon/react';
import './Page.css';
import './Services.css';

const services = [
  {
    id: 'crypto',
    title: 'Cryptocurrency',
    description:
      'Secure digital asset trading and management. Trade popular cryptocurrencies with competitive rates and advanced security measures.',
  },
  {
    id: 'wallet',
    title: 'Digital E-Wallet',
    description:
      'Fast, secure payment infrastructure. Send and receive money instantly with bank-grade encryption and fraud protection.',
  },
  {
    id: 'banking',
    title: 'Banking',
    description:
      'Scalable modern banking solutions. Open an account, manage savings, and access a full suite of banking services.',
  },
  {
    id: 'investing',
    title: 'Investing',
    description:
      'Smart investment tools for wealth building. Access diversified investment portfolios managed by professionals.',
  },
  {
    id: 'loans',
    title: 'Loans',
    description:
      'Flexible lending solutions. Apply for personal or business loans with transparent terms and competitive rates.',
  },
];

export function Services() {
  useEffect(() => {
    window.scrollTo(0, 0);
  }, []);

  return (
    <div className="page-content">
      <div className="page-header">
        <h1>Our Services</h1>
        <p>Comprehensive financial solutions tailored for you</p>
      </div>

      <section className="services-section">
        <div className="services-container">
          <Grid>
            {services.map((service) => (
              <Column key={service.id} sm={4} md={4} lg={6}>
                <Tile id={service.id} className="service-tile">
                  <h3 className="service-title">{service.title}</h3>
                  <p className="service-description">{service.description}</p>
                </Tile>
              </Column>
            ))}
          </Grid>
        </div>
      </section>
    </div>
  );
}
