FROM node:22-alpine3.19

ARG APP_VERSION=development

LABEL org.opencontainers.image.title="T-Skylten / Subway Sign" \
      org.opencontainers.image.description="SL departure display server" \
      org.opencontainers.image.source="https://github.com/LogFlames/subway-sign" \
      org.opencontainers.image.version="${APP_VERSION}"

RUN mkdir -p /home/node/app && chown -R node:node /home/node/app

WORKDIR /home/node/app

COPY package*.json ./

USER node

RUN npm ci

COPY --chown=node:node . .

EXPOSE 8080

CMD [ "npm", "start" ]
