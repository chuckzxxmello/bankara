import { useEffect } from 'react';
import { Grid, Column } from '@carbon/react';
import './Page.css';

export function About() {
  useEffect(() => {
    window.scrollTo(0, 0);
  }, []);

  return (
    <div className="page-content">
      <div className="page-header">
        <h1>About Bankara</h1>
        <p>Leading innovation in fintech solutions</p>
      </div>

      <section className="page-section">
        <Grid>
          <Column sm={4} md={8} lg={12}>
            <h2>Our Mission</h2>
            <p>
              Bankara Fintech Solutions is dedicated to providing cutting-edge
              financial technology that empowers individuals and businesses to
              take control of their financial futures. We believe that financial
              technology should be accessible, secure, and user-friendly.
            </p>

            <h2>Our Values</h2>
            <ul>
              <li>
                <strong>Trustworthiness:</strong> We prioritize security and
                reliability in every aspect of our platform.
              </li>
              <li>
                <strong>Innovation:</strong> We continuously explore new
                technologies to improve our services.
              </li>
              <li>
                <strong>User-Centric Design:</strong> Every feature is designed
                with the user experience in mind.
              </li>
              <li>
                <strong>Transparency:</strong> We maintain clear communication
                with our users about how our systems work.
              </li>
            </ul>

            <h2>Our Team</h2>
            <p>
              Our team consists of experienced professionals in fintech,
              software engineering, and financial services. We are passionate
              about creating products that make a difference in our users&apos;
              lives.
            </p>
          </Column>
        </Grid>
      </section>
    </div>
  );
}
