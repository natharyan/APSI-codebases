"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.sendVerificationEmail = void 0;
const sendEmail_1 = require("./sendEmail");
const sendVerificationEmail = async ({ name, email, verificationToken, origin, }) => {
    const verifyEmail = `${origin}/user/verify-email?token=${verificationToken}&email=${email}`;
    const message = `<p>Please confirm your email by clicking on the following link : 
  <a href="${verifyEmail}">Verify Email</a> </p>`;
    return (0, sendEmail_1.sendEmail)({
        to: email,
        subject: 'Email Confirmation',
        html: `<h4> Hello, ${name}</h4>
    ${message}
    `,
    });
};
exports.sendVerificationEmail = sendVerificationEmail;
