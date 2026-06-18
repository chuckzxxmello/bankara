import { Button } from '@carbon/react';
import './Hero.css';

interface HeroProps {
  onExploreClick: () => void;
}

export function Hero({ onExploreClick }: HeroProps) {
  return (
    <section className="hero">
      <div className="hero-content">
        <h1 className="hero-title">Welcome to Bankara</h1>
        <p className="hero-subtitle">
          Your trusted partner in financial technology
        </p>
        <Button
          kind="primary"
          size="lg"
          onClick={onExploreClick}
          className="hero-cta"
        >
          Explore Our Services
        </Button>
      </div>
    </section>
  );
}
