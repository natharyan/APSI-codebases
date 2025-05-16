"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.sendResetPasswordEmail = void 0;
const sendEmail_1 = require("./sendEmail");
const sendResetPasswordEmail = async ({ name, email, token, origin }) => {
    const resetURL = `${origin}/user/reset-password?token=${token}&email=${email}`;
    const message = `<p>Please reset password by clicking on the following link : 
  <a href="${resetURL}">Reset Password</a></p>`;
    return (0, sendEmail_1.sendEmail)({
        to: email,
        subject: 'Reset Password',
        html: `<h4>Hello, ${name}</h4>
   ${message}
   `,
    });
};
exports.sendResetPasswordEmail = sendResetPasswordEmail;
