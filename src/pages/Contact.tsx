import { useState, useEffect } from 'react';
import { Grid, Column, TextInput, TextArea, Button } from '@carbon/react';
import './Page.css';
import './Contact.css';

export function Contact() {
  const [formData, setFormData] = useState({
    name: '',
    email: '',
    subject: '',
    message: '',
  });

  useEffect(() => {
    window.scrollTo(0, 0);
  }, []);

  const handleChange = (
    e: React.ChangeEvent<HTMLInputElement | HTMLTextAreaElement>
  ) => {
    const { name, value } = e.target;
    setFormData((prev) => ({
      ...prev,
      [name]: value,
    }));
  };

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    console.log('Form submitted:', formData);
    setFormData({
      name: '',
      email: '',
      subject: '',
      message: '',
    });
    alert('Thank you for your message. We will get back to you soon.');
  };

  return (
    <div className="page-content">
      <div className="page-header">
        <h1>Contact Us</h1>
        <p>Get in touch with our team</p>
      </div>

      <section className="contact-section">
        <Grid>
          <Column sm={4} md={4} lg={6}>
            <div className="contact-info">
              <h2>Get in Touch</h2>
              <p>
                Have a question or inquiry? We would love to hear from you.
                Reach out to us using the contact form or the information
                below.
              </p>

              <div className="contact-details">
                <div className="contact-item">
                  <h3>Email</h3>
                  <a href="mailto:info@bankara.com">info@bankara.com</a>
                </div>
                <div className="contact-item">
                  <h3>Phone</h3>
                  <a href="tel:+15551234567">+1 (555) 123-4567</a>
                </div>
              </div>
            </div>
          </Column>

          <Column sm={4} md={4} lg={6}>
            <form onSubmit={handleSubmit} className="contact-form">
              <TextInput
                id="name"
                name="name"
                labelText="Name"
                placeholder="Your name"
                value={formData.name}
                onChange={handleChange}
                required
              />
              <TextInput
                id="email"
                name="email"
                labelText="Email"
                placeholder="your@email.com"
                type="email"
                value={formData.email}
                onChange={handleChange}
                required
              />
              <TextInput
                id="subject"
                name="subject"
                labelText="Subject"
                placeholder="What is this about?"
                value={formData.subject}
                onChange={handleChange}
                required
              />
              <TextArea
                id="message"
                name="message"
                labelText="Message"
                placeholder="Your message..."
                value={formData.message}
                onChange={handleChange}
                rows={6}
                required
              />
              <Button kind="primary" type="submit">
                Send Message
              </Button>
            </form>
          </Column>
        </Grid>
      </section>
    </div>
  );
}
