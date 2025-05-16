"use strict";
// import mongoose from 'mongoose'
// import validator from 'validator'
// import bcrypt from 'bcryptjs'
var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || function (mod) {
    if (mod && mod.__esModule) return mod;
    var result = {};
    if (mod != null) for (var k in mod) if (k !== "default" && Object.prototype.hasOwnProperty.call(mod, k)) __createBinding(result, mod, k);
    __setModuleDefault(result, mod);
    return result;
};
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
// const UserSchema = new mongoose.Schema({
//   name: {
//     type: String,
//     required: [true, 'Please provide name'],
//     minlength: 3,
//     maxlength: 50,
//   },
//   email: {
//     type: String,
//     unique: true,
//     required: [true, 'Please provide email'],
//     validate: {
//       validator: validator.isEmail,
//       message: 'Please provide valid email',
//     },
//   },
//   password: {
//     type: String,
//     required: [true, 'Please provide password'],
//     minlength: 6,
//   },
//   role: {
//     type: String,
//     enum: ['admin', 'user'],
//     default: 'user',
//   },
//   verificationToken: String,
//   isVerified: {
//     type: Boolean,
//     default: false,
//   },
//   verified: Date,
//   passwordToken: {
//     type: String,
//   },
//   passwordTokenExpirationDate: {
//     type: Date,
//   },
// })
// UserSchema.pre('save', async function () {
//   // console.log(this.modifiedPaths())
//   // console.log(this.isModified('name'))
//   if (!this.isModified('password')) return
//   const salt = await bcrypt.genSalt(10)
//   this.password = await bcrypt.hash(this.password, salt)
// })
// UserSchema.methods.comparePassword = async function (canditatePassword: any) {
//   const isMatch = await bcrypt.compare(canditatePassword, this.password)
//   return isMatch
// }
// module.exports = mongoose.model('User', UserSchema)
const mongoose_1 = __importStar(require("mongoose"));
// import validator from 'validator'
const bcryptjs_1 = __importDefault(require("bcryptjs"));
// Define the User schema
const UserSchema = new mongoose_1.Schema({
    name: {
        type: String,
        required: [true, 'Please provide name'],
        minlength: 3,
        maxlength: 50,
    },
    email: {
        type: String,
        unique: true,
        required: [true, 'Please provide email'],
    },
    password: {
        type: String,
        required: [true, 'Please provide password'],
        minlength: 6,
    },
    role: {
        type: String,
        enum: ['admin', 'user'],
        default: 'user',
    },
    verificationToken: String,
    isVerified: {
        type: Boolean,
        default: false,
    },
    verified: Date,
    passwordToken: {
        type: String,
    },
    passwordTokenExpirationDate: {
        type: Date,
    },
});
// Pre-save hook to hash the password if it is modified
UserSchema.pre('save', async function () {
    if (!this.isModified('password'))
        return;
    const salt = await bcryptjs_1.default.genSalt(10);
    this.password = await bcryptjs_1.default.hash(this.password, salt);
});
// Method to compare passwords
UserSchema.methods.comparePassword = async function (candidatePassword) {
    return bcryptjs_1.default.compare(candidatePassword, this.password);
};
// Create and export the User model
const User = mongoose_1.default.model('User', UserSchema);
exports.default = User;
